#include "motor_driver.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"


adc_oneshot_unit_handle_t adc1_handle;


void debug_drive_pins(int pwm_value) {
    // This function is for debugging purposes to test if the PWM output is working correctly based on the supervisor's input.
    // It will set the rpwm pin to the given pwm_value and turn on an LED on GPIO 18 when the supervisor is actively controlling the car.
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_4, pwm_value);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_4);
}

bool is_gas_pedal_pressed() {
    return gpio_get_level(buttonPin) == 0;
}

void kill_motors() {
    // Function to immediately stop the motors, used for emergency stop
    // Occurs when button B is pressed on bluetooth controller
    gpio_set_level(r_en, 0);
    gpio_set_level(l_en, 0);
    
}

void enable_motors() {
    // Function to enable the motors, used after an emergency stop is disengaged
     // Occurs when X+Y are pressed on bluetooth controller
    gpio_set_level(r_en, 1);
    gpio_set_level(l_en, 1);
    
}

void drive_motors(int xPos, int yPos) {
    /* // Arduino code
    if(xPos > 2048) {
    ledcWrite(lsteer, xPos-2048);  //Turn left
    digitalWrite(rsteer, LOW);
  } else if(xPos < 2048) {
    ledcWrite(rsteer, 2048-xPos);  //Turn right
    digitalWrite(lsteer, LOW);
  } else {
    digitalWrite(rsteer, LOW);  // Don't turn
    digitalWrite(lsteer, LOW);
  }

  if (digitalRead(buttonPin) == LOW) {
    if(yPos > 2048) {
     dacWrite(rpwm, (yPos-2048)+50); // Forward
     digitalWrite(lpwm, LOW);
   } else if(yPos < 2048) {
     digitalWrite(rpwm, LOW);   // Reverse
     dacWrite(lpwm, 2048-yPos);
   } else {
     dacWrite(rpwm, 50); //Stop
     digitalWrite(lpwm, LOW);
   }
  } else {
    digitalWrite(rpwm, LOW);  // Stop
    digitalWrite(lpwm, LOW);
  }
    */
    
    // STEERING CONTROL
    if (xPos > 2048 + deadband) {
        // Turn left (Activate lsteer, disable rsteer)
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, xPos - (2048 + deadband));
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0); 
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    } else if (xPos < 2048 - deadband) {
        // Turn right (Activate rsteer, disable lsteer)
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (2048 - deadband) - xPos);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0); 
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

    } else {
        // Don't turn (Disable both)
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    }

    // THROTTLE CONTROL
    // Note: button logic moved to arbitrator.c, so this function only focuses on controlling the motors based on the given xPos and yPos values, which are processed supervisor commands.
    if (yPos > 2048) {
        printf("Forward!\n");
        // Forward (Activate rpwm with offset, disable lpwm)
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, yPos - 2048);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
        
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, 0); 
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3);
    } else if (yPos < 2048) {
        printf("Reverse!\n");
        // Reverse (Activate lpwm, disable rpwm)
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0); 
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
        
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, 2048 - yPos);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3);

    } else {
        // Stop with idle power (rpwm at forwardOffset, lpwm off)
        printf("Idle\n");
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0); 
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
        
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, 0); 
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3);
    }
}

void configure_pins() {
    /*void setup() {
  // put your setup code here, to run once:
    pinMode(rpwm, OUTPUT);
    pinMode(rsteer, OUTPUT);
    pinMode(lpwm, OUTPUT);
    pinMode(lsteer, OUTPUT);
    pinMode(r_en, OUTPUT);
    pinMode(l_en, OUTPUT);

    pinMode(buttonPin, INPUT_PULLUP);
    digitalWrite(r_en, HIGH);
    digitalWrite(l_en, HIGH);

    ledcAttach(rsteer, 5000, 8);
    ledcAttach(lsteer, 5000, 8);
}*/
    gpio_reset_pin(18); // Example pin for debug LED
    gpio_set_direction(18, 2); // Set pin 18 as output

    // Equivalent to pinMode in Arduino, set the motor control pins as outputs
    gpio_reset_pin(rpwm); 
    gpio_reset_pin(rsteer); 
    gpio_reset_pin(lpwm); 
    gpio_reset_pin(lsteer); 
    gpio_reset_pin(r_en); 
    gpio_reset_pin(l_en); 

    gpio_set_direction(rpwm, 2); 
    gpio_set_direction(rsteer, 2); 
    gpio_set_direction(lpwm, 2); 
    gpio_set_direction(lsteer, 2); 
    gpio_set_direction(r_en, 2); 
    gpio_set_direction(l_en, 2);

    // Equivalent to pinMode(buttonPin, INPUT_PULLUP)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << buttonPin),   // Select GPIO pin for the button
        .mode = GPIO_MODE_INPUT,                  // Set as input
        .pull_up_en = GPIO_PULLUP_ENABLE,     // Enable internal pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE        // Disable interrupts
    };

    gpio_config(&io_conf);

    // Set enable pins high to turn on motor drivers
    gpio_set_level(r_en, 1);
    gpio_set_level(l_en, 1);


    // Equivalent to ledcAttach in Arduino, configure 8-bit PWM channels for steering control
    // 1. Configure the Timer (Sets frequency and resolution)
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE, 
        .timer_num        = LEDC_TIMER_0,        // You have 4 timers available (0-3)
        .duty_resolution  = LEDC_TIMER_12_BIT,    // 12-bit resolution (values from 0-4095)
        .freq_hz          = 10000,               // 10000 Hz frequency
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. Configure the Channel (Attaches the timer to your specific pin)
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,        // You have 8 channels available (0-7)
        .timer_sel      = LEDC_TIMER_0,          // Link to the timer we just created
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = rsteer,                // The pin you want to output PWM on
        .duty           = 0,                     // Initial duty cycle (0 = completely off)
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ledc_channel_config_t ledc_channel2 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_1,        // Use a different channel for the second motor
        .timer_sel      = LEDC_TIMER_0,          // Link to the same timer for same frequency
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = lsteer,                // The pin you want to output PWM on
        .duty           = 0,                     // Initial duty cycle (0 = completely off)
        .hpoint         = 0
    };
    
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel2));

    ledc_channel_config_t ledc_channel3 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_2,        // Use a different channel for the second motor
        .timer_sel      = LEDC_TIMER_0,          // Link to the same timer for same frequency
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = rpwm,                // The pin you want to output PWM on
        .duty           = 0,                     // Initial duty cycle (0 = completely off)
        .hpoint         = 0
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel3));

    ledc_channel_config_t ledc_channel4 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_3,        // Use a different channel for the second motor
        .timer_sel      = LEDC_TIMER_0,          // Link to the same timer for same frequency
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = lpwm,                // The pin you want to output PWM on
        .duty           = 0,                     // Initial duty cycle (0 = completely off)
        .hpoint         = 0
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel4));

    // remove later
    ledc_channel_config_t ledc_channel5 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_4,        // Use a different channel for the second motor
        .timer_sel      = LEDC_TIMER_0,          // Link to the same timer for same frequency
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = 18,                // Debug LED pin
        .duty           = 0,                     // Initial duty cycle (0 = completely off)
        .hpoint         = 0
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel5));

    // configure ADC



    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config)); // Configure steeringX (GPIO36), ADC channel 0
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config)); // Configure steeringY (GPIO39), ADC channel 3


}
