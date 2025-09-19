#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#ifndef portTICK_PERIOD_MS
#ifdef configTICK_RATE_HZ
#define portTICK_PERIOD_MS (1000 / configTICK_RATE_HZ)
#else
#define portTICK_PERIOD_MS 1
#endif
#endif

#define NUM_VALUES 20
#define NUM_TRIGGERS 10
#define NUM_FIXTURES 16
#define REFRESH_SEND_HZ 30
#define REFRESH_USB_HZ 60


typedef struct {
    uint8_t values[NUM_VALUES];
    uint8_t triggers[NUM_TRIGGERS];
} payload;


typedef struct {
    uint8_t fixture_id;
    uint8_t mac[6];
    payload data;
} esp_message_t;


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

esp_message_t messages[NUM_FIXTURES];
SemaphoreHandle_t messages_mutex;



// Broadcast MAC address for ESP-NOW

//static const uint8_t ESPNOW_BROADCAST_MAC[ESP_NOW_ETH_ALEN] ={0x78, 0x1C, 0x3C, 0x2B, 0x79, 0xEC};


static SemaphoreHandle_t send_sem;

void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
    // You can still get the peer MAC from info if needed
    // Example:
    // printf("Sent to: %02X:%02X:%02X:%02X:%02X:%02X, status: %d\n",
    //        info->peer_addr[0], info->peer_addr[1], info->peer_addr[2],
    //        info->peer_addr[3], info->peer_addr[4], info->peer_addr[5],
    //        status);

    // Release semaphore to indicate send complete
    xSemaphoreGive(send_sem);
}


void espnow_send_task(void *pvParameter) {
    int wait_time = 1000 / REFRESH_SEND_HZ;
    TickType_t last_wake_time = xTaskGetTickCount();
    int64_t start, end;
    esp_err_t result;
    int sent_per_sec = 0;
    int time_elapsed = 0;
    printf("Starting ESP-NOW send task...\n");

    while (1) {
        start = esp_timer_get_time();

        xSemaphoreTake(messages_mutex, portMAX_DELAY);
        for (int i = 0; i < NUM_FIXTURES; i++) {
            result = esp_now_send(messages[i].mac, (uint8_t*)&messages[i].data, sizeof(messages[i].data));
            if (result != ESP_OK) {
                printf("ESP-NOW send error: %d\n", result);
            } else {
                sent_per_sec++;
                // printf("Sent to fixture %d\n, size: %d\n", messages[i].fixture_id, sizeof(messages[i].data));
                // ✅ Wait until OnDataSent fires before sending next one
                if (xSemaphoreTake(send_sem, pdMS_TO_TICKS(50)) == pdTRUE) {
                    // reset triggers after successful send
                    for (int j = 0; j < NUM_TRIGGERS; j++) {
                        messages[i].data.triggers[j] = 0;
                    }
                } else {
                    printf("Send timeout!\n");
                }
            }
        }
        xSemaphoreGive(messages_mutex);  
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
        end = esp_timer_get_time();
        time_elapsed += (int)(end - start);
        if (time_elapsed > 1000000) {
            printf("Sent %d messages in %d ms\r", sent_per_sec, time_elapsed / 1000);
            sent_per_sec = 0;
            time_elapsed = 0;
        }
    }
}






static int messages_sent = 0;
static int time_elapsed = 0;


void app_main(void) {

    messages_mutex = xSemaphoreCreateMutex();
    if (messages_mutex == NULL) {
        printf("Failed to create mutex!\n");
        return;
    }

    int wait_time = 1000 / REFRESH_USB_HZ;
    TickType_t last_wake_time = xTaskGetTickCount(); // capture current tick

    memcpy(messages[0].mac, peer_mac1, 6);
    messages[0].fixture_id = 1;

    memcpy(messages[1].mac, peer_mac2, 6);
    messages[1].fixture_id = 2;

    memcpy(messages[2].mac, peer_mac3, 6);
    messages[2].fixture_id = 3;

    memcpy(messages[3].mac, peer_mac4, 6);
    messages[3].fixture_id = 4;

    memcpy(messages[4].mac, peer_mac5, 6);
    messages[4].fixture_id = 5;

    memcpy(messages[5].mac, peer_mac6, 6);
    messages[5].fixture_id = 6;

    memcpy(messages[6].mac, peer_mac7, 6);
    messages[6].fixture_id = 7;

    memcpy(messages[7].mac, peer_mac8, 6);
    messages[7].fixture_id = 8;

    memcpy(messages[8].mac, peer_mac9, 6);
    messages[8].fixture_id = 9;

    memcpy(messages[9].mac, peer_mac10, 6);
    messages[9].fixture_id = 10;

    memcpy(messages[10].mac, peer_mac11, 6);
    messages[10].fixture_id = 11;

    memcpy(messages[11].mac, peer_mac12, 6);
    messages[11].fixture_id = 12;

    memcpy(messages[12].mac, peer_mac13, 6);
    messages[12].fixture_id = 13;

    memcpy(messages[13].mac, peer_mac14, 6);
    messages[13].fixture_id = 14;

    memcpy(messages[14].mac, peer_mac15, 6);
    messages[14].fixture_id = 15;

    memcpy(messages[15].mac, peer_mac16, 6);
    messages[15].fixture_id = 16;

    // ESP-NOW and WiFi initialization
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_now_init());
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

    // Start ESP-NOW send task
    //send_sem = xSemaphoreCreateBinary();
    //xTaskCreate(espnow_send_task, "espnow_send_task", 4096, NULL, 5, NULL);
    // esp_now_register_send_cb(OnDataSent);

    // UART configuration
    const int uart_num = UART_NUM_0;
    const int baud_rate = 115200;
    const int buf_size = 2048;
    
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(uart_num, &uart_config);
    uart_driver_install(uart_num, buf_size * 2, 0, 0, NULL, 0);

    uint8_t data[buf_size];
    printf("Listening for serial messages...\n");
    int buf_idx = 0;

    int64_t start, end;

    int len, total, i;
    uint8_t control;
    uint16_t value;
    int8_t fixture_id;
    esp_err_t result;
    uint8_t fixture_changed[NUM_FIXTURES] = {0};

    while (1) {
        start = esp_timer_get_time();
        len = uart_read_bytes(uart_num, data + buf_idx, buf_size - buf_idx, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            total = buf_idx + len;
            i = 0;
            while (i <= total - 5) {
                if (data[i] == 255) {
                    control = data[i+1];
                    value = (data[i+2] << 8) | data[i+3];
                    fixture_id = (int8_t)data[i+4];
                    // printf("Control: %u, Value: %u, Fixture ID: %d\n", control, value, fixture_id);
                    fixture_changed[fixture_id - 1] = 1;

                    if (control < NUM_VALUES) {
                        messages[fixture_id - 1].data.values[control] = value;
                    }
                    else if (control >= NUM_TRIGGERS && control < (NUM_VALUES+NUM_TRIGGERS)) {
                        messages[fixture_id - 1].data.triggers[control - NUM_VALUES] = value;
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
        }
        end = esp_timer_get_time();
        time_elapsed += (int)(end - start);
        if (time_elapsed > 1000000) {
            printf("Sent %d messages in %d ms\r", messages_sent, time_elapsed / 1000);
            messages_sent = 0;
            time_elapsed = 0;
        }
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));

    }
}