
#include "freertos/FreeRTOS.h"
#include "espnow_receiver.h"
#include <stdio.h>
#include <string.h>
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_now.h"
#include "state.h"
#include "utils.h"
#include "set_color.h"



static const char *TAG = "ESPNOW_RECV";

void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    char mac_str[18];
    if (recv_info && recv_info->src_addr) {
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                 recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
    } else {
        strcpy(mac_str, "NULL");
    }
    //ESP_LOGI(TAG, "Received data from %s, len=%d", mac_str, len);
    
    const uint8_t *idata = (const uint8_t *)data;
    
    /*printf("Data (uint8): ");
    for (int i = 0; i < len; i++) {
        printf("%u ", idata[i]);
    }
    printf("\n");
    */
    // Write to global struct if indexes are valid
    if (len > 20) {
        xSemaphoreTake(state_mutex, portMAX_DELAY);
        hsl_to_rgb(idata[0], idata[1], &state.r, &state.g, &state.b);
        hsl_to_rgb(idata[2], idata[3], &state.r_bg, &state.g_bg, &state.b_bg);
        hsl_to_rgb(idata[4], idata[5], &state.r_fast, &state.g_fast, &state.b_fast);

       
        state.f1 = idata[6];
        state.f2 = idata[7];
        state.f3 = idata[8];
        state.D = idata[9];
        state.trail_amount = idata[10];
        state.trail_decay = 1 - 1 / ((float) max(255 - idata[11], 1));
        state.v = idata[12];
        state.noise_level = idata[13];
        state.flow_amount = ((float) idata[14]-127) / 127.0;
        state.decay = 1 - 1 / ((float) max(255 - idata[15], 1));


        xSemaphoreGive(state_mutex);
    
        g_received_data.highlight = idata[21];
        g_received_data.blackout = idata[20];
        g_received_data.flash = idata[22];
        g_received_data.random_pixels = idata[23];
        g_received_data.fast_wave = idata[24];
        g_received_data.noise_flash = idata[25];
    }
}

void espnow_receiver_init(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Wi-Fi in station mode
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Print device's own MAC address
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
    //printf("Device MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

    ESP_LOGI(TAG, "ESP-NOW receiver initialized. Waiting for messages...");
}
 