idf_component_register(SRCS "fixture_id_config.c" "process_pixels.c" "main.c" "espnow_receiver.c" "led_strip_config.c" "utils.c" "set_color.c" "process_pixels"
                    INCLUDE_DIRS "."
                    REQUIRES led_strip esp_timer esp_wifi nvs_flash)