set(srcs "main.c" "my_platform.c" "arbitrator.c" "motor_driver.c")

set(requires "bluepad32" "btstack" "esp_adc")

idf_component_register(SRCS "arbitrator.c" "motor_driver.c" "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES "${requires}")
