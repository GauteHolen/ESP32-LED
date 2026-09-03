#include "setup_uart_rs485.h"
#include "rs485_send_task.h"
#include "crc8.h"
#include <string.h>
#include <stdint.h>
#include "esp_log.h"
#include "config.h"
#include "encoder.h"
#include "message_config.h"
#include "driver/gpio.h"
#include "globals.h"
#include "utils.h"
#include "esp_timer.h"  

static const char *TAG = "RS485_SENDER";

uint8_t frame[RS485_MAX_FRAME_LEN];
uint8_t len_pos;
uint8_t crc;

#define BUFFER_SIZE 1024
static uint8_t tx_buffer[BUFFER_SIZE];
// static fixture_data_t payload = {0};


void rs485_send_task(void *pvParameter){
    char* TAG = "rs485_send_task";

    int wait_time = 1000 / REFRESH_SEND_HZ;
    TickType_t last_wake_time = xTaskGetTickCount();
    int64_t start, end, start_send, end_send;
    

    
    // int time_elapsed = 0;
    sent_per_sec = 0;
    total_send_time = 0;
    send_task_cycles = 0;


    ESP_LOGI(TAG, "Starting ESP-NOW send task...");
    ESP_LOGI(TAG, "Payload size: %d", sizeof(fixture_state_t));
    static fixture_state_t local_payload;
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
                local_payload = broadcast_message;
                reset_all_triggers(&broadcast_message);
                reset_change_flags(&broadcast_message);
                broadcast_message.sequence++;
            give_messages_mutex();
            
            gpio_set_level(LED_PIN, 1);
            
            count_change_flags(&local_payload);

            int message_length = encode_message(&local_payload, tx_buffer, BUFFER_SIZE);
            send_message(tx_buffer, message_length, local_payload.sequence);

            sent_per_sec++;
            set_time_since_change(0);

            vTaskDelay(1);
            reset_change_flags(&local_payload);
            end_send = esp_timer_get_time();
            total_send_time += end_send - start_send;
            
        }
        
        

        //vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
        vTaskDelay(1);
        send_task_cycles++;
    }
}



void send_message(const uint8_t *payload, int payload_len, uint8_t seq)
{
    if (payload == NULL || payload_len < 0) {
        ESP_LOGW(TAG, "Invalid payload");
        return;
    }

    if (payload_len > RS485_MAX_PAYLOAD_LEN) {
        ESP_LOGW(TAG, "Payload too large (%d > %d), dropping frame", payload_len, RS485_MAX_PAYLOAD_LEN);
        return;
    }

    
    int i = 0;

    // ---- sync word ----
    frame[i++] = SYNC1;
    frame[i++] = SYNC2;

    // ---- length (SEQ + DATA) ----
    len_pos = i++;
    
    // ---- sequence ----
    frame[i++] = seq;

    // ---- payload ----
    memcpy(&frame[i], payload, payload_len);
    i += payload_len;

    // ---- fill length ----
    frame[len_pos] = (i - 3); // exclude sync + len byte

    // ---- CRC over (SEQ + DATA) ----
    crc = crc8(&frame[3], i - 3);
    frame[i++] = crc;

    // ---- send ----
    uart_write_bytes(UART_PORT, (const char *)frame, i);

    //ESP_LOGI("UART", "Sent framed packet seq=%d", seq - 1);
}