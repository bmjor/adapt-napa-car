#include "arbitrator.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include "driver/gpio.h"



// this was made for one bts7960, change to fit 3 motor drivers
extern volatile int supervisor_left_pwm;
extern volatile int supervisor_right_pwm;
extern volatile bool supervisor_active;

// put joystick/button input and motor driver functions here

void debug_drive_pins() {
    // Example function to set an LED brightness based on controller PWM value for debugging
    //int brightness = (pwm_value + 255) / 2; // Map -255..255 to 0..255
}

void configure_pins() {
    gpio_reset_pin(18); // Example pin for debug LED
    gpio_set_direction(18, 2); // Set pin 18 as output
}

void car_control_task(void *pvParameters) {
    while (1)
    {    
        // read car input values by calling above functions, e.g. int left_pwm = read_left_joystick();

        // comment out in production
        gpio_set_level(18, supervisor_active); // test bt controller input by lighting up an LED on GPIO 18 when supervisor is active

        if (supervisor_active) { // Supervisor is using bluetooth controller, use controller values
            //drive_motor(supervisor_left_pwm, supervisor_right_pwm); // replace with your motor driver function with bluetooth inputs
            ;
        } else { //  Let the kid drive, use values from on-board controls
            //drive_motor(supervisor_left_pwm, supervisor_right_pwm); // replace with your motor driver function with car inputs
            ;
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Delay every 10ms to let other tasks run and to avoid hogging the CPU
    }
    
}