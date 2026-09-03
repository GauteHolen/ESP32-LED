#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "init_espnow.h"
#include "uart_usb.h"
#include "message_config.h"
#include "driver/gpio.h"
#include "config.h"
#include "espnow_send_task.h"
#include "globals.h"
#include "utils.h"
#include "setup_uart_rs485.h"
#include "rs485_send_task.h"



static int messages_sent = 0;
static int time_elapsed = 0;

void app_main(void) {

    bool rs_485_mode = true; // Set to true to enable RS-485 mode (half-duplex)
    bool espnow_mode = false; // Set to true to enable ESP-NOW mode

    // Led pin FLASH
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    char* TAG = "app_main";



    int wait_time = 1000 / REFRESH_USB_HZ;
    TickType_t last_wake_time = xTaskGetTickCount(); // capture current tick

    create_globals();
    init_fixtures(&broadcast_message);

    // UART configuration
    init_uart();
    uart_stats uart_statistics = {0};
    xTaskCreate(uart_receive_task, "uart_receive_task", 8192, &uart_statistics, configMAX_PRIORITIES-1, NULL);

    gpio_set_level(LED_PIN, 1); // LED on
    vTaskDelay( pdMS_TO_TICKS(1000));
    gpio_set_level(LED_PIN, 0); // LED off
    
    if (rs_485_mode) {
        setup_uart_rs485();
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100));
        xTaskCreate(rs485_send_task, "rs485_send_task", 8192, NULL, configMAX_PRIORITIES-1, NULL);
    }
    else if (espnow_mode) {
        init_espnow(broadcastAddr);
        // Register ESP-NOW send callback
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100));
        ESP_ERROR_CHECK(esp_now_register_send_cb(OnDataSent));
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100));

        xTaskCreate(espnow_send_task, "espnow_send_task", 4096, NULL, configMAX_PRIORITIES-1, NULL);
    }
    else {
        ESP_LOGW(TAG, "No communication mode enabled; set rs_485_mode or espnow_mode to true");
    }


    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(500));
    
    
    int start_copy, end_copy, total_copy = 0;
    int main_cycles = 0;
    int payload_updates = 0;
    int start, end = 0;
    int time_elapsed_received = 0;

    vTaskDelay( pdMS_TO_TICKS(200));
    if (LOG_UART) {
    ESP_LOGI(TAG, "1 Received/s, 2 read maxlen, 3 event count, 4 update payload/s, 5 sent/s, 6 main cycle time: ms, 7 total read time: ms, 8 total copy time: ms, 9 total send time: ms, 10 main cycles/s, 11 send task cycles/s");
    }
    vTaskDelay( pdMS_TO_TICKS(200));

    for (;;) {
        start = esp_timer_get_time();
        // Check if change made
        if (uart_statistics.read_per_cycle > 0) {
            start_copy = esp_timer_get_time();

            copy_uart_to_broadcast_message();
            payload_updates++;
            end_copy = esp_timer_get_time();
            total_copy += end_copy - start_copy;
                
        
       
            set_new_payload(true);
            set_time_since_change(0);

            uart_statistics.read_per_cycle = 0;
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
        main_cycles++;

        end = esp_timer_get_time();
        time_elapsed_received += (end - start);
        increment_time_since_change((int)(end - start));
        if (time_elapsed_received > 1000000 && LOG_UART) {
            //ESP_LOGI(TAG, "Received %d messages, updated payload %d times in %d ms, total read time: %d ms, total copy time: %d ms, total send time: %d ms", received_per_sec, payload_updates, time_elapsed_received / 1000, total_read / 1000, total_copy / 1000, total_send_time / 1000);
            ESP_LOGI(TAG, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", uart_statistics.received_per_sec, uart_statistics.maxlen, uart_statistics.event_count, payload_updates, sent_per_sec, time_elapsed_received / 1000, uart_statistics.total_read / 1000, total_copy / 1000, total_send_time / 1000, main_cycles, send_task_cycles);
            uart_statistics.received_per_sec = 0;
            sent_per_sec = 0;
            time_elapsed_received = 0;
            payload_updates = 0;
            uart_statistics.total_read = 0;
            total_copy = 0;
            total_send_time = 0;
            main_cycles = 0;
            send_task_cycles = 0;
            uart_statistics.maxlen = 0;
            uart_statistics.event_count = 0;
        }
    }
}