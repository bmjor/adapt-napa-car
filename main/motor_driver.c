#include "motor_driver.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"

#define buttonPin 2 // Pin connected to button switch
#define steeringX 36
#define steeringY 39
#define rpwm 25
#define rsteer 32
#define lpwm 26
#define lsteer 33
#define r_en 14
#define l_en 12
#define targetangle 0

void debug_drive_pins() {
    // Example function to set an LED brightness based on controller PWM value for debugging
    //int brightness = (pwm_value + 255) / 2; // Map -255..255 to 0..255
}

void drive_motor(int xPos, int yPos) {
    /*
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

    if (gpio_get_level(buttonPin) == 0) { // Button pressed
        if(yPos > 0) {
            // Forward
            gpio_set_level(rpwm, 1); 
            gpio_set_level(lpwm, 1); 
        } else if(yPos < 0) {
            // Reverse
            gpio_set_level(rpwm, 0); 
            gpio_set_level(lpwm, 0); 
        } else {
            // Stop
            gpio_set_level(rpwm, 0);
            gpio_set_level(lpwm, 0);
        }
    } else {
        // Button not pressed, stop the motors
        gpio_set_level(rpwm, 0);
        gpio_set_level(lpwm, 0);
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
    gpio_reset_pin(buttonPin);
    gpio_input_enable(buttonPin); 
    gpio_pullup_en(buttonPin); // Enable internal pull-up resistor

    // Set enable pins high to turn on motor drivers
    gpio_set_level(r_en, 1);
    gpio_set_level(l_en, 1);


    // Equivalent to ledcAttach in Arduino, configure PWM channels for steering control
    // 1. Configure the Timer (Sets frequency and resolution)
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE, 
        .timer_num        = LEDC_TIMER_0,        // You have 4 timers available (0-3)
        .duty_resolution  = LEDC_TIMER_8_BIT,    // 8-bit resolution (values from 0-255)
        .freq_hz          = 5000,                // 5000 Hz frequency
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


    // configure ADC
    adc_oneshot_unit_handle_t adc1_handle;
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
