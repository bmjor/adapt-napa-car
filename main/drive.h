#ifndef DRIVE_H
#define DRIVE_H

#include <stdbool.h>
#include <stdint.h>

// Config
#define WITH_BT_CONTROLLER 1

typedef struct Position {
    int16_t x; // both values should range from -100 to 100
    int16_t y;
} Position;


typedef struct {
    bool connected;
    bool emergency_stop;
    bool hard_override_on;
    int16_t thrust;
    int16_t steering;
} supervisor_cmd_t;


void setup_driving(void); 
void drive_set_supervisor_connected();
void drive_on_supervisor_disconnect(void); 
void drive_set_supervisor_emergency_stop(bool on);
void drive_set_supervisor_override(bool on);
void drive_set_supervisor_speed(int16_t steering, int16_t thrust);

#endif
