idf_component_register(SRCS "main.c" "setup_uart.c" "sender.c" "crc8.c" "encoder.c"
                    REQUIRES nvs_flash esp_driver_uart
                    INCLUDE_DIRS ".")
