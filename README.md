# Adapt NAPA Car - ESP32 Control System

A custom embedded control system for a joystick adapted ride-on car for the NAPA Center. 

This project replaces the original electronics of a commercial ride-on car with an ESP32 microcontroller, which interfaces with an on-board joystick to replace the steering wheel. It utilizes the Bluepad32 library to allow physical therapists to safely supervise and override the child's driving using a standard Bluetooth controller.

## Control Scheme (Bluetooth)

<img width="2230" height="1002" alt="control_scheme_temp(1)" src="https://github.com/user-attachments/assets/8dba97a7-5819-4ac1-ab24-8239b5cd7b00" />

On a standard controller, the left joystick maps to forwards/backwards motion(rear motors) and the right joystick maps to horizontal motion(front motor). Button Y engages the emergency stop, and Button A disengages it. These controls can be changed in my_platform.c

## Hardware Setup

* **Microcontroller:** ESP32 Development Board
* **Motor Drivers:** 2x BTS7960 43A High-Power H-Bridges (1x Steering, 1x Rear Drive connected in parallel to LEDC channels).
* **Child Inputs:** Analog Potentiometer Joystick (Steering) & Button (Horn).
* **Supervisor Input:** Any standard Bluetooth Gamepad (via Bluepad32).

## Build and Flash

This project is built using the **Espressif IoT Development Framework (ESP-IDF)**.

1. Install the ESP-IDF toolchain (v4.4 or v5.x).
2. Clone this repository.
3. Build the project:
   ```bash
   idf.py build
