#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"m
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "drive.h"
#include "motor_driver.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "drive"; // For logs

static void drive_task(void *pvParameter);

// Inputs
#define BUTTON_PIN 15 // PIN CONNECTED TO BUTTON SWITCH
#define JOYSTICK_X 36 // JOYSTICK X DIRECTION (STEERING) (Channel 0 ADC)
#define JOYSTICK_Y 39 // JOYSTICK Y DIRECTION (THRUST) (Channel 3 ADC)

// Outputs
#define FRONT_MOTOR_LEFT_PWM_PIN 32  // LEFT PWM
#define FRONT_MOTOR_RIGHT_PWM_PIN 33 // RIGHT PWM
#define REAR_MOTOR_LEFT_PWM_PIN 25 // LEFT PWM
#define REAR_MOTOR_RIGHT_PWM_PIN 26 // RIGHT PWM
#define R_EN 14 // CONNECTED TO ALL DRIVERS' R_EN PINS IN PARALLEL
#define L_EN 12 // CONNECTED TO ALL DRIVERS' L_EN PINS IN PARALLEL

// Constants
#define JOYSTICK_DEADBAND 50
#define CONTROLLER_DEADBAND 10
#define MAX_FORWARD 60 // CHANGE
#define MAX_BACKWARD 35
#define FORWARD_SHUTOFF_THRESOLD 15 // % TEST AND CHANGE
#define BACKWARD_SHUTOFF_THRESOLD 15 // % TEST AND CHANGE
#define SPEED_INCREMENT 0.5f // % of increment per loop

// Channels
#define REAR_MOTOR_PWM_CHANNEL_FORWARD LEDC_CHANNEL_0
#define REAR_MOTOR_PWM_CHANNEL_BACKWARD LEDC_CHANNEL_1
#define FRONT_MOTOR_PWM_CHANNEL_FORWARD LEDC_CHANNEL_2
#define FRONT_MOTOR_PWM_CHANNEL_BACKWARD LEDC_CHANNEL_3
#define MOTOR_PWM_TIMER LEDC_TIMER_1
#define MOTOR_PWM_DUTY_RESOLUTION LEDC_TIMER_10_BIT
#define JOYSICK_X_ADC_CHANNEL ADC_CHANNEL_0
#define JOYSICK_Y_ADC_CHANNEL ADC_CHANNEL_3


// Variables in memory
static uint32_t adc_x_average = 0;
static uint32_t adc_x_voltage = 0;
static uint32_t adc_y_average = 0;
static uint32_t adc_y_voltage = 0;

static float thrust_speed = 0;
static float steering_speed = 0;

// Local supervisor state
static supervisor_cmd_t current_supervisor_state = {
    .connected = false,
    .emergency_stop = false,
    .hard_override_on = false,
    .thrust = 0,
    .steering = 0,
};

////////////////////
// SETUP
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);

void setup_pins(void) {

    // ADC1 Init
    adc_oneshot_unit_handle_t adc1_handle; // adc 2 interferes with bluetooth
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };  
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));  

    // ADC1 Config
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOYSICK_X_ADC_CHANNEL, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOYSICK_Y_ADC_CHANNEL, &config));

    // ADC Calibration Init
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    adc_cali_handle_t adc1_cali_chan3_handle = NULL;
    bool do_calibration1_chan0 = adc_calibration_init(ADC_UNIT_1, JOYSICK_X_ADC_CHANNEL, ADC_ATTEN_DB_12, &adc1_cali_chan0_handle);
    bool do_calibration1_chan1 = adc_calibration_init(ADC_UNIT_1, JOYSICK_Y_ADC_CHANNEL, ADC_ATTEN_DB_12, &adc1_cali_chan3_handle);

    // Other pins
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_PIN);

    gpio_reset_pin(R_EN); 
    gpio_set_direction(R_EN, GPIO_MODE_OUTPUT); 

    gpio_reset_pin(L_EN); 
    gpio_set_direction(L_EN, GPIO_MODE_OUTPUT);
    
    gpio_set_level(R_EN, 0); // Disable motors before enabling pwm
    gpio_set_level(L_EN, 0);

    gpio_reset_pin(FRONT_MOTOR_RIGHT_PWM_PIN);
    gpio_set_direction(FRONT_MOTOR_RIGHT_PWM_PIN, GPIO_MODE_OUTPUT);

    gpio_reset_pin(FRONT_MOTOR_LEFT_PWM_PIN);
    gpio_set_direction(FRONT_MOTOR_LEFT_PWM_PIN, GPIO_MODE_OUTPUT);

    gpio_reset_pin(REAR_MOTOR_RIGHT_PWM_PIN);
    gpio_set_direction(REAR_MOTOR_RIGHT_PWM_PIN, GPIO_MODE_OUTPUT);

    gpio_reset_pin(REAR_MOTOR_LEFT_PWM_PIN);
    gpio_set_direction(REAR_MOTOR_LEFT_PWM_PIN, GPIO_MODE_OUTPUT);

    gpio_set_level(R_EN, 1); 
    gpio_set_level(L_EN, 1);
}
void setup_pwm(void) {
    // PWM
    ledc_channel_config_t ledc_channel_s_forward = {0}, ledc_channel_s_backward = {0};
    ledc_channel_config_t ledc_channel_t_forward = {0}, ledc_channel_t_backward = {0};
    
    // Figure out what these mean
    ledc_channel_s_forward.gpio_num = FRONT_MOTOR_RIGHT_PWM_PIN; // Right corresponds to forwards
    ledc_channel_s_forward.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel_s_forward.channel = FRONT_MOTOR_PWM_CHANNEL_FORWARD;
    ledc_channel_s_forward.intr_type = LEDC_INTR_DISABLE;
    ledc_channel_s_forward.timer_sel = MOTOR_PWM_TIMER;
    ledc_channel_s_forward.duty = 0;

    ledc_channel_s_backward.gpio_num = FRONT_MOTOR_LEFT_PWM_PIN; // Left corresponds to reverse
    ledc_channel_s_backward.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel_s_backward.channel = FRONT_MOTOR_PWM_CHANNEL_BACKWARD;
    ledc_channel_s_backward.intr_type = LEDC_INTR_DISABLE;
    ledc_channel_s_backward.timer_sel = MOTOR_PWM_TIMER;
    ledc_channel_s_backward.duty = 0;

    ledc_channel_t_forward.gpio_num = REAR_MOTOR_RIGHT_PWM_PIN; // Right corresponds to forwards
    ledc_channel_t_forward.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel_t_forward.channel = REAR_MOTOR_PWM_CHANNEL_FORWARD;
    ledc_channel_t_forward.intr_type = LEDC_INTR_DISABLE;
    ledc_channel_t_forward.timer_sel = MOTOR_PWM_TIMER;
    ledc_channel_t_forward.duty = 0;

    ledc_channel_t_backward.gpio_num = REAR_MOTOR_LEFT_PWM_PIN; // Left corresponds to reverse
    ledc_channel_t_backward.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel_t_backward.channel = REAR_MOTOR_PWM_CHANNEL_BACKWARD;
    ledc_channel_t_backward.intr_type = LEDC_INTR_DISABLE;
    ledc_channel_t_backward.timer_sel = MOTOR_PWM_TIMER;
    ledc_channel_t_backward.duty = 0;


    ledc_timer_config_t ledc_timer = {0}; 
    ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_timer.duty_resolution = MOTOR_PWM_DUTY_RESOLUTION;
    ledc_timer.timer_num = MOTOR_PWM_TIMER;
    ledc_timer.freq_hz = 25000;

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_s_forward));
	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_s_backward));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_t_forward));
	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_t_backward));
	ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
}

void setup_driving(void) {
    // Setup Pins
    setup_pins();

    // Setup PWM
    setup_pwm();
    
    // Create Drive Task with high priority(20), and 2048 words allocated for its stack.
    xTaskCreate(&drive_task, "drive_task", 4096, NULL, 2, NULL);
    
    // Implement Interrupt-driven horn
}

static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}


////////////////////
// LOGIC


float compute_next_speed(float current, float target, float delta) {
    if (current < target) {
        // Forward direction -> Car is accelerating
        // Backwards direction -> Car is deccelerating
        if (current < 0 && current > -BACKWARD_SHUTOFF_THRESOLD) {
            return 0;
        }
        else if (current > 0 && current  && current < FORWARD_SHUTOFF_THRESOLD) {
            return FORWARD_SHUTOFF_THRESOLD;
        }
        else if(current < 0) {
            float slowdown_rate = current > 50 ? 0.08 : 0.04;
            return current + delta * slowdown_rate;
        }
        else {
            return current + SPEED_INCREMENT;
        }
        if (current > target) return target; // Check for overshooting
    }
    else if (current > target) { 
        // Forward direction -> Car is deccelerating
        // Backwards direction -> Car is accelerating
        if (current > 0 && current < FORWARD_SHUTOFF_THRESOLD) {
            return 0;
        }
        else if (current < 0 && current > -BACKWARD_SHUTOFF_THRESOLD) {
            return -BACKWARD_SHUTOFF_THRESOLD;
        }
        else if (current > 0) {
            float slowdown_rate = current > 50 ? 0.08 : 0.04;
            return current - delta * slowdown_rate;
        }
        else {
            return current - SPEED_INCREMENT;
        }
    }
    if (current < target) return target; // Check for overshooting

    return current;
}

// Returns a Postion type with both x and y between -100 and 100. positive y is forwards, positive x is right. Multiply by -1 if direction is incorrect.
// The position type is located in drive.h
// TODO: make sure a deadband exists. That is, within a certain range, specified by JOYSTICK_DEADBAND, it should always return 0. This is to ensure noise or small movements do not result in the motors moving.
// Note: Use abs(placeholder_ADC_value) > JOYSTICK_DEADBAND to check whether the joystick is outside the deadband

Position get_joystick_position(uint8_t gpioX, uint8_t gpioY) {
    Position js_position = {0,0};
    return js_position;
}

void send_values_to_motors(int16_t steering_speed, int16_t thrust_speed) {
    // Calculate duty cycles from speed inputs(-100 to 100)
    float s_forward_duty_fraction = 0;
    float s_backward_duty_fraction = 0;

    if (steering_speed > 0) {
        s_forward_duty_fraction = steering_speed / 100.0f;
    } else if (steering_speed < 0) {
        s_backward_duty_fraction = steering_speed / 100.0f;
    }

    uint32_t s_max_duty = (1 << MOTOR_PWM_DUTY_RESOLUTION) - 1;
    uint32_t s_forward_duty = lroundf(s_forward_duty_fraction * (float)s_max_duty);
    uint32_t s_backward_duty = -1 * lroundf(s_backward_duty_fraction * (float)s_max_duty);

    float t_forward_duty_fraction = 0;
    float t_backward_duty_fraction = 0;

    if (thrust_speed > 0) {
        t_forward_duty_fraction = thrust_speed / 100.0f;
    } else if (thrust_speed < 0) {
        t_backward_duty_fraction = thrust_speed / 100.0f;
    }

    uint32_t t_max_duty = (1 << MOTOR_PWM_DUTY_RESOLUTION) - 1;
    uint32_t t_forward_duty = lroundf(t_forward_duty_fraction * (float)t_max_duty);
    uint32_t t_backward_duty = -1 * lroundf(t_backward_duty_fraction * (float)t_max_duty);
    
    // Send to motors
    // Front motor(steering)
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, FRONT_MOTOR_PWM_CHANNEL_FORWARD, s_forward_duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, FRONT_MOTOR_PWM_CHANNEL_FORWARD));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, FRONT_MOTOR_PWM_CHANNEL_BACKWARD, s_backward_duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, FRONT_MOTOR_PWM_CHANNEL_BACKWARD)); 

    // Back motors(thrust)
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, REAR_MOTOR_PWM_CHANNEL_FORWARD, t_forward_duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, REAR_MOTOR_PWM_CHANNEL_FORWARD));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, REAR_MOTOR_PWM_CHANNEL_BACKWARD, t_backward_duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, REAR_MOTOR_PWM_CHANNEL_BACKWARD)); 

}

// Supervisor disconnect handler
void drive_on_supervisor_disconnect(void) {
    current_supervisor_state.connected = false;
    current_supervisor_state.hard_override_on = false; // change maybe? If disconnected, the child should have no control?
    current_supervisor_state.steering = 0;
    current_supervisor_state.thrust = 0; // Test to see if spinlock is neccesary
}

// Supervisor state updates
void drive_set_supervisor_connected() {
    current_supervisor_state.connected = true;
}

void drive_set_supervisor_override(bool on) { 
    current_supervisor_state.hard_override_on = on; // If hard override is set, the child has no control, only supervisor
}

void drive_set_supervisor_emergency_stop(bool on){
    current_supervisor_state.emergency_stop = on;
}

void drive_set_supervisor_speed(int16_t steering, int16_t thrust) {
    current_supervisor_state.steering = steering;
    current_supervisor_state.thrust = thrust;
}


////////////////////
// TASKS
static void drive_task(void *pvParameter) {
    int64_t last_update = esp_timer_get_time();
    float delta;

    Position joystick = {0,0};


    while (true) {
#if WITH_BT_CONTROLLER
        // ESP_LOGI(TAG,
        //          "Supervisor: connected=%d emergency_stop=%d override=%d steering=%d thrust=%d",
        //          current_supervisor_state.connected,
        //          current_supervisor_state.emergency_stop,
        //          current_supervisor_state.hard_override_on,
        //          (unsigned int)current_supervisor_state.steering,
        //          (unsigned int)current_supervisor_state.thrust);
        // vTaskDelay(pdMS_TO_TICKS(1000)); // For debugging
        if (current_supervisor_state.emergency_stop) {
            thrust_speed = 0;
            steering_speed = 0;
            
            send_values_to_motors(steering_speed, thrust_speed); // harsh stop

            last_update = esp_timer_get_time();

            vTaskDelay(50 / portTICK_PERIOD_MS);
            continue;
        }

        delta = (esp_timer_get_time() - last_update) / 1000;

        // Compute speeds
        if (current_supervisor_state.connected) {
            
            // check override
            if (current_supervisor_state.hard_override_on) {
                joystick.x = 0, joystick.y = 0; // disable child controls
            } else joystick = get_joystick_position(JOYSTICK_X, JOYSTICK_Y);
            
            // if supervisor uses controller, it should always override.
            if (abs(current_supervisor_state.thrust) > CONTROLLER_DEADBAND) { 
                thrust_speed = compute_next_speed(thrust_speed, current_supervisor_state.thrust, delta);
            } else thrust_speed = compute_next_speed(thrust_speed, joystick.x, delta);

            if (abs(current_supervisor_state.steering) > CONTROLLER_DEADBAND) {
                steering_speed = compute_next_speed(steering_speed, current_supervisor_state.steering, delta);
            } else steering_speed = compute_next_speed(steering_speed, joystick.y, delta);

        }
        else {
           // set motors to 0 -> smooth stop.
           // Set R_en and L_en to 0 to enable soft brake? test to see if necessary, or if software slowing is enough. 
            thrust_speed = compute_next_speed(thrust_speed, 0, delta);
            steering_speed = compute_next_speed(steering_speed, 0, delta);
            
        }
        

#else
        // run based on joystick values

        delta = (esp_timer_get_time() - last_update) / 1000;

        joystick = get_joystick_position(joystickX, joystickY);

        thrust_speed = compute_next_speed(thrust_speed, joystick.x, delta);
        steering_speed = compute_next_speed(steering_speed, joystick.y, delta);
        
#endif
        // Send to motors
        send_values_to_motors(steering_speed, thrust_speed);

        last_update = esp_timer_get_time();

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// LED Task -> use software timer instead

// Sound Task -> keep input handling in drive_task, but if playback requires blocking, make a task for that. otherwise trigger in drive task as well.

// Do we actually need tasks for these? 