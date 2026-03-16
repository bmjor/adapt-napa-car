#include "arbitrator.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include "motor_driver.h"
#include "esp_adc/adc_oneshot.h"

// this was made for one bts7960, change to fit 3 motor drivers
extern volatile int supervisor_xPos;
extern volatile int supervisor_yPos;
extern volatile bool supervisor_active;


// Freertos task that continuously reads the supervisor state and controls the car accordingly
void car_control_task(void *pvParameters) {
    while (1)
    {    
        int xPos = 0;
        int yPos = 0;

        //ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &xPos));
        //ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &yPos));

        // comment out in production
        //gpio_set_level(18, supervisor_active); // test bt controller input by lighting up an LED on GPIO 18 when supervisor is active
        
        // Supervisor is using bluetooth controller, use controller values
        if (supervisor_active) { 
            drive_motor(supervisor_xPos, supervisor_yPos); 
        } else { //  Let the kid drive, use values from on-board controls
            drive_motor(xPos, yPos);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Delay every 10ms to let other tasks run and to avoid hogging the CPU
    }
    
}