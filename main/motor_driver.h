#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

// Function prototypes for motor driver functions
void drive_motors(int xPos, int yPos);
void configure_pins();
void debug_drive_pins(int pwm_value);
void kill_motors();
void enable_motors();

#endif // MOTOR_DRIVER_H