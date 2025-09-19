idf_component_register(SRCS "trigger_events.c" "main.c" "led_strip_config.c"
                    INCLUDE_DIRS "."
                    REQUIRES led_strip esp_timer esp_wifi nvs_flash)