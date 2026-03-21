#include "arbitrator.h"

extern adc_oneshot_unit_handle_t adc1_handle; // Declare the ADC handle as extern to use it in this file

// Slew Rate Limiter For Smooth Acceleration/Deceleration
static int16_t apply_slew_limit(int16_t target_val, int16_t current_val, int16_t max_change) {
    // 1. Compute how far we need to move
    int32_t delta = target_val - current_val;
    
    // 2. Clamp the change to our maximum allowed acceleration/deceleration
    if (delta > max_change) {
        delta = max_change;
    } else if (delta < -max_change) {
        delta = -max_change;
    }
    
    // 3. Return the new smoothed output
    return current_val + (int16_t)delta;
}

// Freertos task that continuously reads the supervisor state and controls the car accordingly
void car_control_task(void *pvParameters) {
    int16_t current_motor_x = 2048; // Start at neutral position
    int16_t current_motor_y = 2048; // Start at neutral position

    // At 100Hz loop speed, 20 units per loop = 1.0 second from 100% to 0%.
    int max_step = 20;

    supervisor_cmd_t sup_cmd = {
        .x = 2048,
        .y = 2048,
        .hard_override = false,
        .emergency_stop = false,
        .is_connected = false
    };
    
    while (1)
    {
        xQueueReceive(supervisor_queue, &sup_cmd, 0);
        
        if (sup_cmd.is_connected == false) {
            // If the controller is not connected, ensure the motors are stopped and skip the rest of the loop
            kill_motors();
            vTaskDelay(pdMS_TO_TICKS(10)); // Delay to avoid busy looping while waiting for controller connection
            continue;
        }

        int child_x = 0;
        int child_y = 0;
        
        // Read the ADC values for steeringX and steeringY and store them in child_x and child_y
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &child_x));
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &child_y));
        
        // for testing when joystick is not wired
        child_x = 2048; 
        child_y = 1048;
        
        // Determine if the supervisor is actively moving the joystick (soft override) even if the trigger is not pulled
        bool supervisor_soft_override = (sup_cmd.x > 2248 || sup_cmd.x < 1848 || 
                                         sup_cmd.y > 2248 || sup_cmd.y < 1848);

        int target_motor_x = 2048; // Default to stop
        int target_motor_y = 2048; 

        if (sup_cmd.emergency_stop) {
            // PRIORITY 0: Emergency Stop. If engaged, it overrides EVERYTHING and forces the car to stop immediately.
            // Physically kill power to motor drivers with the enable pins
            printf("EMERGENCY STOP ENGAGED!\n");
            kill_motors();
            // Zero out the PWM for extra safety
            current_motor_x = 2048;
            current_motor_y = 2048;
            target_motor_x = 2048;
            target_motor_y = 2048;
        } else {
            // SYSTEM NORMAL: Re-enable the motor drivers
            enable_motors();

            if (sup_cmd.hard_override) {
                // PRIORITY 1: Trigger is pulled. Supervisor takes FULL control. 
                // Even if the supervisor joystick is centered, it will force the car to stop.
                printf("Hard Override Engaged!\n");
                target_motor_x = sup_cmd.x;
                target_motor_y = sup_cmd.y;

            } else if (supervisor_soft_override) {
                // PRIORITY 2: Trigger is NOT pulled, but supervisor is moving the joystick.
                // Override the kid with the supervisor's movement.
                printf("Soft Override Engaged!\n");
                target_motor_x = sup_cmd.x;
                target_motor_y = sup_cmd.y;

            } else {
                // PRIORITY 3: Supervisor is doing nothing. Kid has full control.
                if (child_x < 15 || child_x > 4081 || child_y < 15 || child_y > 4081) {
                    // If the ADC reads an impossible extreme, a wire is unplugged or broken.
                    // Force all motors to stop immediately
                    printf("ADC Reading Out of Bounds! Stopping motors for safety\n");
                    current_motor_x = 2048;
                    current_motor_y = 2048;
                    target_motor_x = 2048;
                    target_motor_y = 2048;
                }

                if (!is_gas_pedal_pressed()) {
                    // If the gas pedal is not pressed, ignore the kid's joystick input and keep the car stationary
                    printf("Gas Pedal Not Pressed! Ignoring kid's joystick input\n");
                    child_x = 2048;
                    child_y = 2048;
                }
                else if (child_y >= 2048){
                    child_y += forwardOffset;
                    printf("Gas Pedal Pressed! Child Y with forward offset: %d\n", child_y);
                }
                target_motor_x = child_x;
                target_motor_y = child_y;
            }
            

        }
        
        if (!sup_cmd.emergency_stop) {
            // Apply slew rate limiting to the motor outputs for smooth acceleration/deceleration
            current_motor_x = apply_slew_limit(target_motor_x, current_motor_x, max_step);
            current_motor_y = apply_slew_limit(target_motor_y, current_motor_y, max_step);
        }

        // Send the final motor commands to the motor driver
        drive_motors(current_motor_x, current_motor_y);

        
        // remove later
        //debug_drive_pins(target_motor_x);
        //printf("Output: (%d, %d) | Supervisor: (%d, %d) | Child: (%d, %d) | Emergency Stop: %d\n", 
                //current_motor_x, current_motor_y, sup_cmd.x, sup_cmd.y, child_x, child_y, sup_cmd.emergency_stop);

        vTaskDelay(pdMS_TO_TICKS(10)); // Delay every 10ms to let other tasks run and to avoid hogging the CPU
    }
    
}