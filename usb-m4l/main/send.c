#include "message_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_now.h"
#include "esp_log.h"

#define LOG_SEND_TRIGGER false
#define LOG_SEND_VALUES false

char *TAG = "send";
esp_err_t result;

void reset_fixture_triggers(fixture_message_t *fixture) {
    if (fixture != NULL) {
        for (int i = 0; i < NUM_TRIGGERS; i++) {
            fixture->data.triggers[i] = 0;
        }
    }
}

void reset_all_triggers(broadcast_message_t *message) {
    if (message != NULL) {
        for (int f = 0; f < NUM_FIXTURES; f++) {
            reset_fixture_triggers(&message->fixtures[f]);
        }
    }
}

int send_broadcast_message(broadcast_message_t *message, const uint8_t *broadcastAddr, SemaphoreHandle_t send_sem) {
    

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

void init_fixtures(broadcast_message_t *message) {
    for (uint8_t i = 0; i < NUM_FIXTURES; i++) {
        message->fixtures[i].fixture_id = (uint8_t) i + 1;
        for (uint8_t j = 0; j < NUM_VALUES; j++) {
            message->fixtures[i].data.values[j] = (uint8_t) 0;
        }
        for (uint8_t j = 0; j < NUM_TRIGGERS; j++) {
            message->fixtures[i].data.triggers[j] = (uint8_t) 0;
        }

    }
}