#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>


void init_espnow(uint8_t *broadcastAddr) {
    TickType_t last_wake_time = xTaskGetTickCount(); // capture current tick
    char* TAG = "init_espnow";
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
    ESP_LOGI(TAG, "ESP-NOW initialized");

    ESP_LOGI(TAG, "Broadcast MAC: %02X:%02X:%02X:%02X:%02X:%02X",
         broadcastAddr[0], broadcastAddr[1], broadcastAddr[2],
         broadcastAddr[3], broadcastAddr[4], broadcastAddr[5]);
    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(200));
    // Add broadcast MAC as peer
    esp_now_peer_info_t peerInfo = {0};
    memcpy(peerInfo.peer_addr,broadcastAddr, ESP_NOW_ETH_ALEN);
    peerInfo.channel = 0;
    peerInfo.ifidx = ESP_IF_WIFI_STA;
    peerInfo.encrypt = false;


    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(200));
    esp_err_t err = esp_now_add_peer(&peerInfo);
    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "%s", esp_err_to_name(err));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Broadcast peer added");
    } else if (err == ESP_ERR_ESPNOW_EXIST) {
        ESP_LOGW(TAG, "Broadcast peer already exists");
    } else {
        ESP_LOGE(TAG, "esp_now_add_peer failed: %s", esp_err_to_name(err));
    }

    // Start ESP-NOW send task
    // send_sem = xSemaphoreCreateBinary();

}