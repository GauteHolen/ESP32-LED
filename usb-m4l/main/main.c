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
#include "init_uart.h"
#include "message_config.h"
#include "driver/gpio.h"

#ifndef portTICK_PERIOD_MS
#ifdef configTICK_RATE_HZ
#define portTICK_PERIOD_MS (1000 / configTICK_RATE_HZ)
#else
#define portTICK_PERIOD_MS 1
#endif
#endif
#define LED_PIN 2

#define REFRESH_SEND_HZ 60
#define REFRESH_USB_HZ 200






// fixture_message_t messages[NUM_FIXTURES];
static SemaphoreHandle_t messages_mutex;
static broadcast_message_t broadcast_message = (broadcast_message_t){.sequence = 0, .magic = {'N','A','B','O'}};
static uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static esp_now_peer_info_t broadcastPeer;


// Broadcast MAC address for ESP-NOW

//static const uint8_t ESPNOW_BROADCAST_MAC[ESP_NOW_ETH_ALEN] ={0x78, 0x1C, 0x3C, 0x2B, 0x79, 0xEC};


static SemaphoreHandle_t send_sem;
// Semaphore to protect access to value_changed (static binary semaphore)
static bool value_changed = false;
static StaticSemaphore_t value_changed_sem_buf;
static SemaphoreHandle_t value_changed_sem = NULL;
static int time_since_change = 0;


static uint8_t uart_rx_buf[8192];


void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
    // You can still get the peer MAC from info if needed
    // Example:
    // printf("Sent to: %02X:%02X:%02X:%02X:%02X:%02X, status: %d\n",
    //        info->peer_addr[0], info->peer_addr[1], info->peer_addr[2],
    //        info->peer_addr[3], info->peer_addr[4], info->peer_addr[5],
    //        status);

    // Release semaphore to indicate send complete (if available)
    if (send_sem != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGive(send_sem);
        (void)xHigherPriorityTaskWoken;
    } else {
        ESP_LOGW("OnDataSent", "send_sem is NULL, cannot give semaphore");
    }
}


void espnow_send_task(void *pvParameter) {
    char* TAG = "espnow_send_task";

    int wait_time = 1000 / REFRESH_SEND_HZ;
    TickType_t last_wake_time = xTaskGetTickCount();
    int64_t start, end, start_send, end_send;
    esp_err_t result;
    int sent_per_sec = 0;
    int time_elapsed = 0;
    ESP_LOGI(TAG, "Starting ESP-NOW send task...");
    ESP_LOGI(TAG, "Payload size: %d", sizeof(broadcast_message));

    for (;;) {

        start = esp_timer_get_time();

        // read value_changed atomically
        bool local_value_changed = false;
        if (value_changed_sem != NULL) {
            xSemaphoreTake(value_changed_sem, portMAX_DELAY);
            local_value_changed = value_changed;
            xSemaphoreGive(value_changed_sem);
        } else {
            local_value_changed = value_changed;
        }

        if (!local_value_changed && time_since_change < 2000000) {
            vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));

        } else {
            // Acquire in consistent order: value_changed_sem then messages_mutex
            if (value_changed_sem != NULL) {
                xSemaphoreTake(value_changed_sem, portMAX_DELAY);
            }
            // ESP_LOGI(TAG, "Sequence: %d, Sending updated values...", broadcast_message.sequence);
            xSemaphoreTake(messages_mutex, portMAX_DELAY);
            //start_send = esp_timer_get_time();
            for (int f = 0; f < NUM_FIXTURES; f++) {
                for (int j = 0; j < NUM_TRIGGERS; j++) {
                    if (broadcast_message.fixtures[f].data.triggers[j] != 0) {
                        printf("Fixture %d, Trigger %d: %d\n",
                                 f + 1, j + 1, broadcast_message.fixtures[f].data.triggers[j]);
                    }
                }
            } 
            gpio_set_level(LED_PIN, 1);
            result = esp_now_send(broadcastAddr,
                (uint8_t*)&broadcast_message, 
                sizeof(broadcast_message));
            if (result != ESP_OK) {
                ESP_LOGW(TAG, "ESP-NOW send error: %d", result);
            } 
            else if (send_sem != NULL) {
                
                
                // Wait for send completion callback (if semaphore created)

                if (xSemaphoreTake(send_sem, pdMS_TO_TICKS(100)) == pdTRUE) {
                    broadcast_message.sequence++;
                    time_since_change = 0;
                    sent_per_sec++;
                    
                        //end_send = esp_timer_get_time();
                    //ESP_LOGI(TAG, "Send time: %d us", (int)(end_send - start_send)/1000);
                    // messages_sent++;
                    // reset triggers after successful send
                    for (int f = 0; f < NUM_FIXTURES; f++) {
                        for (int j = 0; j < NUM_TRIGGERS; j++) {
                        broadcast_message.fixtures[f].data.triggers[j] = 0;
                        }
                    }
                } else {
                    ESP_LOGW(TAG, "Send timeout - callback not received!");
                }
            } else {
                ESP_LOGW(TAG, "send_sem not created; skipping wait for send callback");
            }

            // reset change flag while still holding semaphores
                value_changed = false;
                xSemaphoreGive(messages_mutex);
                if (value_changed_sem != NULL) {
                    xSemaphoreGive(value_changed_sem);
                }
            }

            vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
            gpio_set_level(LED_PIN, 0);
            end = esp_timer_get_time();
            time_elapsed += (int)(end - start);

            if (time_elapsed > 1000000) {
                ESP_LOGI(TAG, "Sent %d messages in %d ms\r", sent_per_sec, time_elapsed / 1000);
                sent_per_sec = 0;
                time_elapsed = 0;
        }   
    }
}






static int messages_sent = 0;
static int time_elapsed = 0;

void app_main(void) {

    char* TAG = "app_main";

    messages_mutex = xSemaphoreCreateMutex();
    if (messages_mutex == NULL) {
        ESP_LOGI(TAG, "Failed to create mutex!");
        return;
    }

    int wait_time = 1000 / REFRESH_USB_HZ;
    TickType_t last_wake_time = xTaskGetTickCount(); // capture current tick

    for (uint8_t i = 0; i < NUM_FIXTURES; i++) {
        broadcast_message.fixtures[i].fixture_id = (uint8_t) i + 1;
        for (uint8_t j = 0; j < NUM_VALUES; j++) {
            broadcast_message.fixtures[i].data.values[j] = (uint8_t) 0;
        }
        for (uint8_t j = 0; j < NUM_TRIGGERS; j++) {
            broadcast_message.fixtures[i].data.triggers[j] = (uint8_t) 0;
        }

    }


    init_espnow(broadcastAddr);

    // Register ESP-NOW send callback
    ESP_ERROR_CHECK(esp_now_register_send_cb(OnDataSent));

    // Create semaphore used to wait for espnow send completion
    send_sem = xSemaphoreCreateBinary();
    if (send_sem == NULL) {
        ESP_LOGW(TAG, "Failed to create send_sem; espnow_send_task will skip waiting for callbacks");
    }

    // Create static binary semaphore to protect value_changed
    value_changed_sem = xSemaphoreCreateBinaryStatic(&value_changed_sem_buf);
    if (value_changed_sem == NULL) {
        ESP_LOGW(TAG, "Failed to create value_changed_sem");
    } else {
        // ensure semaphore is in the 'available' state for use as a mutex-like guard
        xSemaphoreGive(value_changed_sem);
    }

    xTaskCreate(espnow_send_task, "espnow_send_task", 4096, NULL, 5, NULL);




    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(500));
    // UART configuration
    const int uart_num = UART_NUM_0;
    const int baud_rate = 2000000; // 921600 bps; ensure host/CH340 support this
    const int buf_size = 8192; // larger buffer to absorb bursts
    init_uart(uart_num, baud_rate, buf_size);

    // Led pin FLASH
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 1); // LED on
    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(2000));
    gpio_set_level(LED_PIN, 0); // LED off

    // Use file-scope buffer to avoid large stack allocation
    uint8_t *data = uart_rx_buf;
    ESP_LOGI(TAG,  "Listening for serial messages...");
    int buf_idx = 0;

    int64_t start, end;

    int len, total, i;
    uint8_t control;
    uint16_t value;
    int8_t fixture_id;
    esp_err_t result;
    uint8_t fixture_changed[NUM_FIXTURES] = {0};
    /* initialize fixtures buffer to zero to avoid copying uninitialized memory */
    fixture_message_t fixtures[NUM_FIXTURES] = {0};

    uint8_t received_per_sec = 0;
    int time_elapsed_received = 0;
    int payload_updates = 0;


    for (;;) {
    start = esp_timer_get_time();
    /* shorter timeout to reduce per-call latency; read more often */
    len = uart_read_bytes(uart_num, data + buf_idx, buf_size - buf_idx, 2 / portTICK_PERIOD_MS);
        if (len > 0) {
            total = buf_idx + len;
            i = 0;
            while (i <= total - 5) {
                if (data[i] == 255) {
                    control = data[i+1];
                    value = (data[i+2] << 8) | data[i+3];
                    fixture_id = (int8_t)data[i+4];
                    // printf("Control: %u, Value: %u, Fixture ID: %d\n", control, value, fixture_id);
                    

                    // Acquire protection in consistent order: value_changed_sem then messages_mutex
                    if (value_changed_sem != NULL) {
                        xSemaphoreTake(value_changed_sem, portMAX_DELAY);
                    }

                    // validate fixture_id before indexing
                    if (fixture_id < 1 || fixture_id > NUM_FIXTURES) {
                        ESP_LOGW("app_main", "Received out-of-range fixture_id: %d", fixture_id);
                    } else {
                        int idx = fixture_id - 1;
                        received_per_sec++;
                        if (control < NUM_VALUES) {
                            fixtures[idx].data.values[control] = (uint8_t)value;
                        }
                        else if (control >= NUM_VALUES && control < (NUM_VALUES + NUM_TRIGGERS)) {
                            fixtures[idx].data.triggers[control - NUM_VALUES] = (uint8_t)value;
                        }
                    }
                    value_changed = true;
                    time_since_change = 0;

                    
                    if (value_changed_sem != NULL) {
                        xSemaphoreGive(value_changed_sem);
                    }
                    i += 5;
                } else {
                    i++;
                }
            }
            // Move any leftover bytes to the start of the buffer
            buf_idx = total - i;
            if (buf_idx > 0 && i < total) {
                memmove(data, data + i, buf_idx);
            }

            
        }
        if (xSemaphoreTake(messages_mutex, portMAX_DELAY) == pdTRUE) {
            for (int f = 0; f < NUM_FIXTURES; f++) {
                /* copy only the payload (values+triggers) to avoid overwriting adjacent memory
                   fixtures[f].data and broadcast_message.fixtures[f].data are of type payload */
                memcpy(&broadcast_message.fixtures[f].data.values, &fixtures[f].data.values, sizeof(fixtures[f].data.values));
                for (int j = 0; j < NUM_TRIGGERS; j++) {
                    if (fixtures[f].data.triggers[j] != 0) {
                    // Copy
                    broadcast_message.fixtures[f].data.triggers[j] = fixtures[f].data.triggers[j];
                    // reset 
                    fixtures[f].data.triggers[j] = 0; 
                    }
                    
                }
                
                
                
            }
            xSemaphoreGive(messages_mutex);
            payload_updates++;
        }
        
        end = esp_timer_get_time();
        time_elapsed_received += (int)(end - start);
        time_since_change += (int)(end - start);
        if (time_elapsed_received > 1000000) {
            ESP_LOGI(TAG, "Received %d messages, updated payload %d times in %d ms", received_per_sec, payload_updates, time_elapsed_received / 1000);
            received_per_sec = 0;
            time_elapsed_received = 0;
            payload_updates = 0;
        }
        vTaskDelay(1);
        //vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));

    }
}



/*
uint8_t peer_mac1[6] = {0x78, 0x1C, 0x3C, 0x2B, 0x79, 0xEC};
uint8_t peer_mac2[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xEC};
uint8_t peer_mac3[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xED};
uint8_t peer_mac4[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xEE};
uint8_t peer_mac5[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xEF};
uint8_t peer_mac6[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xF0};
uint8_t peer_mac7[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xF1};
uint8_t peer_mac8[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xF2};
uint8_t peer_mac9[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xF3};
uint8_t peer_mac10[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xF4};
uint8_t peer_mac11[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xF5};
uint8_t peer_mac12[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xF6};
uint8_t peer_mac13[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xF7};
uint8_t peer_mac14[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xF8};
uint8_t peer_mac15[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xF9};
uint8_t peer_mac16[6] = {0x74, 0x1C, 0x3C, 0x2B, 0x79, 0xFA};


for (int i = 0; i < NUM_FIXTURES; i++) {
        esp_now_peer_info_t peerInfo = {0};
        memcpy(peerInfo.peer_addr, messages[i].mac, ESP_NOW_ETH_ALEN);
        peerInfo.channel = 0;
        peerInfo.ifidx = ESP_IF_WIFI_STA;
        peerInfo.encrypt = false;
        esp_err_t err = esp_now_add_peer(&peerInfo);
        if (err == ESP_ERR_ESPNOW_EXIST) {
            // Peer already exists, optionally update with esp_now_mod_peer(&peerInfo);
        } else {
            ESP_ERROR_CHECK(err);
        }
    }



// send data
            for (int i = 0; i < NUM_FIXTURES; i++) {
                if (!fixture_changed[i]) {
                    continue; // skip if no change
                }
                fixture_changed[i] = 0; // reset change flag
                result = esp_now_send(messages[i].mac, (uint8_t*)&messages[i].data, sizeof(messages[i].data));
                if (result == ESP_OK) {
                    for (int j = 0; j < NUM_TRIGGERS; j++) {
                        messages[i].data.triggers[j] = 0;
                    }
                    messages_sent++;
                }
            }

*/