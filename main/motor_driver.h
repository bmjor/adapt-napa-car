#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include "stdbool.h"

#define buttonPin 14 // Pin connected to button switch
#define steeringX 36 
#define steeringY 39
#define rpwm 25
#define rsteer 32
#define lpwm 26
#define lsteer 33
#define r_en 14 // connected to all drivers' R_EN pins in parallel
#define l_en 12 // connected to all drivers' L_EN pins in parallel
#define targetangle 0
#define deadband 50
#define forwardOffset 100 // Offset so that when the child presses the gas pedal, the car moves forward


// Function prototypes for motor driver functions
void drive_motors(int xPos, int yPos);
void configure_pins();
void debug_drive_pins(int pwm_value);
void kill_motors();
void enable_motors();
bool is_gas_pedal_pressed();

#endif // MOTOR_DRIVER_H