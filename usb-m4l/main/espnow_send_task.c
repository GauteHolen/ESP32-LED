#include "config.h"
#include "espnow_send_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "globals.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "message_config.h"
#include "uart_usb.h"
#include "driver/gpio.h"
#include "esp_now.h"
#include "utils.h"


#define LOG_SEND_TRIGGER false
#define LOG_SEND_VALUES false


esp_err_t result;
esp_message_t esp_msg = {.magic = MAGIC_ESPNOW};


int send_task_cycles = 0;
int total_send_time = 0;
int sent_per_sec = 0;

SemaphoreHandle_t send_sem;


void create_send_sem(){
    send_sem = xSemaphoreCreateBinary();
    if (send_sem == NULL) {
        ESP_LOGW("send_sem", "Failed to create send_sem; espnow_send_task will skip waiting for callbacks");
    }
}

void espnow_send_task(void *pvParameter) {

    create_send_sem();

    char* TAG = "espnow_send_task";

    int wait_time = 1000 / REFRESH_SEND_HZ;
    TickType_t last_wake_time = xTaskGetTickCount();
    int64_t start, end, start_send, end_send;
    

    
    // int time_elapsed = 0;
    sent_per_sec = 0;
    total_send_time = 0;
    send_task_cycles = 0;


    ESP_LOGI(TAG, "Starting ESP-NOW send task...");
    ESP_LOGI(TAG, "Payload size: %d", sizeof(broadcast_message));
    fixture_state_t local_msg;
    bool local_new_payload = false;
    int result=0;
    int local_time_since_change = 0;  

    for (;;) {
        // ESP_LOGI(TAG, "turning off led");
        gpio_set_level(LED_PIN, 0);

        start = esp_timer_get_time();
        local_time_since_change = get_time_since_change();
        local_new_payload = get_new_payload();




        if (!local_new_payload && local_time_since_change < 1000000) {
            vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
            //vTaskDelay();
            continue;

        } else {

            set_new_payload(false);
            local_new_payload = false;

            // Send updated values via ESP-NOW
            start_send = esp_timer_get_time();

            // ESP_LOGI(TAG, "Sequence: %d, Sending updated values...", broadcast_message.sequence);
            take_messages_mutex();
                local_msg = broadcast_message;
                reset_all_triggers(&broadcast_message);
                broadcast_message.sequence++;
            give_messages_mutex();
            
            gpio_set_level(LED_PIN, 1);

            fixture_data_to_esp_message(&local_msg, &esp_msg);

            result = send_broadcast_message(&esp_msg, broadcastAddr, send_sem);
            if (result == 200) {
                sent_per_sec++;
                set_time_since_change(0);
            }
            else {
                ESP_LOGE(TAG, "Failed to send message, error code: %d", result);
                set_time_since_change(0);
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



int send_broadcast_message(esp_message_t *message, const uint8_t *broadcastAddr, SemaphoreHandle_t send_sem) {
    char *TAG = "send";

    if (LOG_SEND_TRIGGER) {
        ESP_LOGI(TAG, "Log message triggers:");
        for (int i = 0; i < NUM_FIXTURES; i++) {
            // Sanity check fixture IDs
            printf("\nFixture ID: %d", message->fixtures[i].fixture_id);
            for (int j = 0; j < NUM_TRIGGERS; j++) {
                printf( "\t%d", message->fixtures[i].data.triggers[j]);
                }
            }
        printf("\n");
    }
    if (LOG_SEND_VALUES) {
        ESP_LOGI(TAG, "Log message values:");
        for (int i = 0; i < NUM_FIXTURES; i++) {
            // Sanity check fixture IDs
            printf("\nFixture ID: %d", message->fixtures[i].fixture_id);
            for (int j = 0; j < NUM_VALUES; j++) {
                printf( "\t%d", message->fixtures[i].data.values[j]);
                }
            }
        printf("\n");
    }
   
    // ESP_LOGI(TAG, "Message length: %d bytes", sizeof(*message));
    
    result = esp_now_send(broadcastAddr, (uint8_t*)message,sizeof(*message));
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "ESP-NOW send error: %s (%d)", esp_err_to_name(result), result);
        return 1;
    } 
    
    if (send_sem != NULL) {
        // Wait for send completion callback (if semaphore created)
        if (xSemaphoreTake(send_sem, pdMS_TO_TICKS(20)) == pdTRUE) {
            
            return 200; // Success
        } else {
            ESP_LOGW(TAG, "Send timeout - callback not received!");
        }
    }
    return 2; // Timeout or no semaphore
}


void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {

    //ESP_LOGI("OnDataSent", "Send callback: status=%s", (status == ESP_NOW_SEND_SUCCESS) ? "SUCCESS" : "FAIL");

    if (send_sem != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGive(send_sem);
        (void)xHigherPriorityTaskWoken;
    } else {
        ESP_LOGW("OnDataSent", "send_sem is NULL, cannot give semaphore");
    }
}