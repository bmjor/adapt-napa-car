set(srcs "main.c" "my_platform.c" "arbitrator.c")

set(requires "bluepad32" "btstack")

idf_component_register(SRCS "arbitrator.c" "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES "${requires}")
