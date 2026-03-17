# Adapt NAPA Car - ESP32 Control System

A custom embedded control system for a modified ride-on car for the NAPA Center. 

This project replaces the factory electronics(JR1958RX-2S) with an ESP32 microcontroller running a FreeRTOS architecture. It utilizes the Bluepad32 library to allow physical therapists to safely supervise and override the child's driving using a standard Bluetooth gamepad.

## System Architecture

The core of the software is the **Priority Arbitrator**, running continuously at 100Hz on a dedicated CPU core (Core 1). It securely multiplexes inputs between the child's physical controls (joystick/pedal) and the therapist's Bluetooth controller, ensuring safe handoffs without race conditions.

### Priority Levels
1. **Priority 0 (E-Stop Latched):** Total system lockdown. Hardware enable pins are pulled LOW.
2. **Priority 1 (Hard Override):** Therapist pulls the controller trigger. Child controls are completely ignored.
3. **Priority 2 (Soft Override):** Therapist moves the Bluetooth joystick. Child controls are temporarily ignored until the therapist releases the stick.
4. **Priority 3 (Child Driving):** System normal. The child has full control of the vehicle.

## Safety Features

Because this vehicle is used by children with limited trunk stability and mobility, software safety is the primary focus:

* **Hardware-Level Latching E-Stop:** Pressing the designated E-Stop button instantly pulls the `R_EN` and `L_EN` pins of the BTS7960 drivers LOW, physically severing power to the motors. The car enters a latched state and cannot be driven again until a specific two-button combo is pressed by the therapist.
* **Feedforward Slew Rate Limiter (Anti-Jerk):** Implements a mathematical deceleration/acceleration ramp. This prevents "whiplash" if the child floors the pedal, and provides a smooth "ease stop" when letting off the gas, protecting both the passenger and the plastic gearboxes.
* **Open-Circuit Failsafe:** The ESP32 continuously monitors the child's ADC joystick lines. If a wire vibrates loose or snaps, the system detects the impossible `0` or `4095` voltage reading and immediately immobilizes the car.
* **Steering Torque Cap:** Because the factory steering column lacks limit switches, a software PWM cap prevents the BTS7960 driver from dumping destructive stall currents into the steering motor when the wheels hit their physical turn radius limit.
* **Idle Deadband & Creep:** Includes an adjustable joystick deadband to prevent drift, alongside an optional low-PWM baseline offset to mimic the natural "idle creep" of an automatic transmission vehicle.

## Control Scheme (Bluetooth)

To keep operation simple for therapists, the system is designed to pair with a standard Bluetooth controller (e.g., Xbox Wireless Controller) using color-coded inputs:

* **Drive/Steer:** Left/Right Thumbsticks
* **Hard Override:** Right Trigger (Hold)
* **Emergency Stop:** `B` Button (Red) - *Latches the car in a safe state.*
* **Unlock System:** Hold `X` (Blue) + `Y` (Yellow) simultaneously.

## Hardware Setup

* **Microcontroller:** ESP32 Development Board
* **Motor Drivers:** 3x BTS7960 43A High-Power H-Bridges (1x Steering, 2x Rear Drive connected in parallel to LEDC channels).
* **Child Inputs:** Analog Potentiometer Joystick (Steering) & Button (Throttle).
* **Supervisor Input:** Any standard Bluetooth Gamepad (via Bluepad32).

## Build and Flash

This project is built using the **Espressif IoT Development Framework (ESP-IDF)**.

1. Install the ESP-IDF toolchain (v4.4 or v5.x).
2. Clone this repository.
3. Build the project:
   ```bash
   idf.py build
