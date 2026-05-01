idf_component_register(SRCS "main.c" "receiver.c" "crc8.c" "setup_uart.c" "decoder.c"
                    REQUIRES nvs_flash esp_driver_uart esp_system
                    INCLUDE_DIRS ".")
