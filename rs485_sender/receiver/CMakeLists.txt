idf_component_register(SRCS "receiver.c"
                    REQUIRES nvs_flash esp_driver_uart
                    INCLUDE_DIRS ".")
