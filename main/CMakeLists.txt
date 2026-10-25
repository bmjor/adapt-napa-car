set(srcs "main.c" "my_platform.c")

set(requires "bluepad32" "btstack" "esp_adc")

idf_component_register(SRCS "drive.c" "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES "${requires}")
