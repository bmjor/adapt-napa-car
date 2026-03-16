// Example file - Public Domain
// Need help? http://bit.ly/bluepad32-help

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <btstack_port_esp32.h>
#include <btstack_run_loop.h>
#include <btstack_stdio_esp32.h>
#include <hci_dump.h>
#include <hci_dump_embedded_stdout.h>
#include <uni.h>

#include "sdkconfig.h"
#include "arbitrator.h"
#include "motor_driver.h"

// Sanity check
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Must use BLUEPAD32_PLATFORM_CUSTOM"
#endif


// Defined in my_platform.c
struct uni_platform* get_my_platform(void);

int app_main(void) {

    
    // If you enable HCI Dump better to disable "Bluepad32 USB Console" from "idf.py menuconfig".
    // hci_dump_init(hci_dump_embedded_stdout_get_instance());

    // Don't use BTstack buffered UART. It conflicts with the console.
#ifdef CONFIG_ESP_CONSOLE_UART
#ifndef CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE
    btstack_stdio_init();
#endif  // CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE
#endif  // CONFIG_ESP_CONSOLE_UART

    // Configure BTstack for ESP32 VHCI Controller
    btstack_init();
    
    // Must be called before uni_init()
    uni_platform_set_custom(get_my_platform());

    // Init Bluepad32.
    uni_init(0 /* argc */, NULL /* argv */);

    // temp: initialize pins for debugging
    configure_pins();

    // Creates and assigns motor control task to core 1, so that it doesn't interfere with Bluetooth on core 0
    xTaskCreatePinnedToCore(car_control_task, "car_control", 4096, NULL, 5, NULL, 1); 

    // Does not return
    btstack_run_loop_execute(); 

    return 0;
    
}
