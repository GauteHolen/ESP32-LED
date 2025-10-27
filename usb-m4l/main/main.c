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
#include "uart.h"
#include "message_config.h"
#include "driver/gpio.h"
#include "send.h"


#ifndef portTICK_PERIOD_MS
#ifdef configTICK_RATE_HZ
#define portTICK_PERIOD_MS (1000 / configTICK_RATE_HZ)
#else
#define portTICK_PERIOD_MS 1
#endif
#endif
#define LED_PIN 2
#define LOG_UART false

#define REFRESH_SEND_HZ 200
#define REFRESH_USB_HZ 500







static SemaphoreHandle_t messages_mutex;
static broadcast_message_t broadcast_message = (broadcast_message_t){.sequence = 0, .magic = {'N','A','B','O'}};
static uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static esp_now_peer_info_t broadcastPeer;


// Broadcast MAC address for ESP-NOW

//static const uint8_t ESPNOW_BROADCAST_MAC[ESP_NOW_ETH_ALEN] ={0x78, 0x1C, 0x3C, 0x2B, 0x79, 0xEC};


static SemaphoreHandle_t send_sem;
// Semaphore to protect access to new_payload (static binary semaphore)
static bool new_payload = false;
static StaticSemaphore_t new_payload_sem_buf;
static SemaphoreHandle_t new_payload_sem = NULL;
static int time_since_change = 0;





void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {

    if (send_sem != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGive(send_sem);
        (void)xHigherPriorityTaskWoken;
    } else {
        ESP_LOGW("OnDataSent", "send_sem is NULL, cannot give semaphore");
    }
}


int send_task_cycles = 0;
int total_send_time = 0;
int sent_per_sec = 0;

void espnow_send_task(void *pvParameter) {
    char* TAG = "espnow_send_task";

    int wait_time = 1000 / REFRESH_SEND_HZ;
    TickType_t last_wake_time = xTaskGetTickCount();
    int64_t start, end, start_send, end_send;
    

    
    int time_elapsed = 0;
    ESP_LOGI(TAG, "Starting ESP-NOW send task...");
    ESP_LOGI(TAG, "Payload size: %d", sizeof(broadcast_message));
    broadcast_message_t local_msg;
    bool local_new_payload = false;
    int result=0;

    for (;;) {
        // ESP_LOGI(TAG, "turning off led");
        gpio_set_level(LED_PIN, 0);

        start = esp_timer_get_time();
        

        // read new_payload atomically
        if (xSemaphoreTake(new_payload_sem, portMAX_DELAY) == pdTRUE) {
            local_new_payload = new_payload;
            xSemaphoreGive(new_payload_sem);
        }


        if (!local_new_payload && time_since_change < 5000000) {
            vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
            //vTaskDelay();
            continue;

        } else {

            if (xSemaphoreTake(new_payload_sem, portMAX_DELAY) == pdTRUE) {
                new_payload = false;
            xSemaphoreGive(new_payload_sem);
            }
            local_new_payload = false;

            // Send updated values via ESP-NOW
            start_send = esp_timer_get_time();

            // ESP_LOGI(TAG, "Sequence: %d, Sending updated values...", broadcast_message.sequence);
            xSemaphoreTake(messages_mutex, portMAX_DELAY);
                local_msg = broadcast_message;
                reset_all_triggers(&broadcast_message);
                broadcast_message.sequence++;
            xSemaphoreGive(messages_mutex);
            
            gpio_set_level(LED_PIN, 1);

            result = send_broadcast_message(&local_msg, broadcastAddr, send_sem);
            if (result == 200) {
                sent_per_sec++;
                time_since_change = 0;
            }
            

            
            
            end_send = esp_timer_get_time();
            total_send_time += end_send - start_send;
        }
        
        

        //vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
        vTaskDelay(1);
        send_task_cycles++;


        /*end = esp_timer_get_time();
        time_elapsed += (int)(end - start);

        if (time_elapsed > 1000000) {
            ESP_LOGI(TAG, "Sent %d messages in %d ms\r", sent_per_sec, time_elapsed / 1000);
            sent_per_sec = 0;
            time_elapsed = 0;
        }   
        */

    }
}






static int messages_sent = 0;
static int time_elapsed = 0;

void app_main(void) {

    // Led pin FLASH
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    char* TAG = "app_main";

    messages_mutex = xSemaphoreCreateMutex();
    if (messages_mutex == NULL) {
        ESP_LOGI(TAG, "Failed to create mutex!");
        return;
    }

    int wait_time = 1000 / REFRESH_USB_HZ;
    TickType_t last_wake_time = xTaskGetTickCount(); // capture current tick

    init_fixtures(&broadcast_message);
    init_espnow(broadcastAddr);

    // Register ESP-NOW send callback
    ESP_ERROR_CHECK(esp_now_register_send_cb(OnDataSent));

    // Create semaphore used to wait for espnow send completion
    send_sem = xSemaphoreCreateBinary();
    if (send_sem == NULL) {
        ESP_LOGW(TAG, "Failed to create send_sem; espnow_send_task will skip waiting for callbacks");
    }

    // Create static binary semaphore to protect new_payload
    new_payload_sem = xSemaphoreCreateBinaryStatic(&new_payload_sem_buf);
    if (new_payload_sem == NULL) {
        ESP_LOGW(TAG, "Failed to create new_payload_sem");
    } else {
        // ensure semaphore is in the 'available' state for use as a mutex-like guard
        xSemaphoreGive(new_payload_sem);
    }

    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(500));
    // UART configuration
    
    init_uart();

    
    gpio_set_level(LED_PIN, 1); // LED on
    vTaskDelay( pdMS_TO_TICKS(2000));
    gpio_set_level(LED_PIN, 0); // LED off

    xTaskCreate(espnow_send_task, "espnow_send_task", 4096, NULL, configMAX_PRIORITIES-1, NULL);

    uart_stats uart_statistics = {0};
    xTaskCreate(uart_receive_task, "uart_receive_task", 8192, &uart_statistics, configMAX_PRIORITIES-1, NULL);

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
            if (xSemaphoreTake(messages_mutex, portMAX_DELAY) == pdTRUE) {
                start_copy = esp_timer_get_time();
                for (int f = 0; f < NUM_FIXTURES; f++) {
                    /* copy only the payload (values+triggers) to avoid overwriting adjacent memory
                    fixtures[f].data and broadcast_message.fixtures[f].data are of type payload */
                    memcpy(&broadcast_message.fixtures[f].data.values, &fixtures[f].data.values, sizeof(fixtures[f].data.values));
                    for (int j = 0; j < NUM_TRIGGERS; j++) {
                        if (fixtures[f].data.triggers[j] > 0) {
                        // Copy
                        broadcast_message.fixtures[f].data.triggers[j] = fixtures[f].data.triggers[j];
                        // reset 
                        
                        }
                    }
                }
                xSemaphoreGive(messages_mutex);
                payload_updates++;
                end_copy = esp_timer_get_time();
                total_copy += end_copy - start_copy;
                vTaskDelay(1); // wait for copy before resetting triggers
                for (int f = 0; f < NUM_FIXTURES; f++) {
                    for (int j = 0; j < NUM_TRIGGERS; j++) {
                        fixtures[f].data.triggers[j] = 0;
                    }
                }
            }
       
            xSemaphoreTake(new_payload_sem, portMAX_DELAY);
            new_payload = true;
            time_since_change = 0;
            xSemaphoreGive(new_payload_sem);
            uart_statistics.read_per_cycle = 0;
        }



        

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
        main_cycles++;

        end = esp_timer_get_time();
        time_elapsed_received += (end - start);
        time_since_change += (int)(end - start);
        if (time_elapsed_received > 1000000 && LOG_UART) {
            //ESP_LOGI(TAG, "Received %d messages, updated payload %d times in %d ms, total read time: %d ms, total copy time: %d ms, total send time: %d ms", received_per_sec, payload_updates, time_elapsed_received / 1000, total_read / 1000, total_copy / 1000, total_send_time / 1000);
            printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", uart_statistics.received_per_sec, uart_statistics.maxlen, uart_statistics.event_count, payload_updates, sent_per_sec, time_elapsed_received / 1000, uart_statistics.total_read / 1000, total_copy / 1000, total_send_time / 1000, main_cycles, send_task_cycles);
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