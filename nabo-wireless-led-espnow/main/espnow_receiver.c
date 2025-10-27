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
#include "message_config.h" 
#include "fixture_id_config.h"
#include "esp_timer.h"
#include "driver/gpio.h"

int start, end;

uint8_t sequence = 0;



static const char *TAG = "ESPNOW_RECV";

// Helper: expected size of broadcast_message_t on the wire
#define BROADCAST_MESSAGE_SIZE (4 /*magic*/ + 1 /*sequence*/ + NUM_FIXTURES * (1 + NUM_VALUES + NUM_TRIGGERS))

// Validate magic bytes (accept any non-zero magic; change as needed)
static bool validate_magic(const uint8_t *magic)
{
    // Expect ASCII "NABO"
    if (!magic) return false;
    if (magic[0] == 'N' && magic[1] == 'A' && magic[2] == 'B' && magic[3] == 'O') {
        return true;
    }
    ESP_LOGW(TAG, "Invalid magic bytes: %02X %02X %02X %02X (expected 'NABO')", magic[0], magic[1], magic[2], magic[3]);
    return false;
}

// Try to parse incoming buffer into broadcast_message_t. Returns true if parsed and valid.
static bool parse_broadcast_message(const uint8_t *buf, int len, broadcast_message_t *out)
{
    if (!buf || !out) return false;
    if (len < (int)BROADCAST_MESSAGE_SIZE) return false;

    // memcpy is safe because message is only uint8_t arrays and small integers
    memcpy(out, buf, BROADCAST_MESSAGE_SIZE);

    if (!validate_magic(out->magic)) return false;
    return true;
}

void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{   
    gpio_set_level(LED_PIN, 1); // LED on
    received_per_second++;
    start = esp_timer_get_time();
    

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
    // First: try parsing as a broadcast_message_t
    if (len >= BROADCAST_MESSAGE_SIZE) {
        broadcast_message_t msg;
        if (parse_broadcast_message(idata, len, &msg)) {
            // ESP_LOGI(TAG, "Received broadcast seq=%u from %s", msg.sequence, mac_str);
            // Debug: print received length and magic
            //ESP_LOGI(TAG, "Packet len=%d, expected broadcast size=%d", len, BROADCAST_MESSAGE_SIZE);
            //ESP_LOGI(TAG, "Magic bytes: %02X %02X %02X %02X", msg.magic[0], msg.magic[1], msg.magic[2], msg.magic[3]);

            // Find fixture entry that matches this device's fixture_id
            int found_idx = -1;
            // Compute the expected offset of fixture i in the raw buffer
            const int fixture_entry_size = 1 + NUM_VALUES + NUM_TRIGGERS; // fixture_id + values + triggers
            const int fixtures_base = 4 + 1; // magic (4) + sequence (1)

            
            for (int i = 0; i < NUM_FIXTURES; i++) {
                uint8_t parsed_id = msg.fixtures[i].fixture_id;
                int raw_offset = fixtures_base + i * fixture_entry_size;
                uint8_t raw_id = 0xFF;
                if (raw_offset < len) raw_id = idata[raw_offset];
                // ESP_LOGI(TAG, "Fixture %d: parsed_id=%u raw_buffer[%d]=%02X", i, parsed_id, raw_offset, raw_id);
                if (parsed_id == FIXTURE_ID) {
                    found_idx = i;
                    break;
                }
            }

            if (sequence != msg.sequence) {
                ESP_LOGE(TAG, "Seq mismatch: was %u, now %u", sequence, msg.sequence);
                sequence = msg.sequence;
            }
            sequence++;

            if (found_idx >= 0) {
                // Copy payload into state
                xSemaphoreTake(state_mutex, portMAX_DELAY);
                payload *p = &msg.fixtures[found_idx].data;
                // Map some values into state (guard indices)
                hsl_to_rgb(p->values[0], p->values[1], &state.r, &state.g, &state.b);
                hsl_to_rgb(p->values[2], p->values[3], &state.r_bg, &state.g_bg, &state.b_bg);
                hsl_to_rgb(p->values[4], p->values[5], &state.r_fast, &state.g_fast, &state.b_fast);
                state.f1 = p->values[6];
                state.f2 = p->values[7];
                state.f3 = p->values[8];
                state.D = p->values[9];
                state.trail_amount = p->values[10];
                state.trail_decay = 1 - 1 / ((float) max(255 - p->values[11], 1));
                state.v = p->values[12];
                state.noise_level = p->values[13];
                state.flow_amount = ((float) p->values[14]-127) / 127.0;
                state.decay = 1 - 1 / ((float) max(255 - p->values[15], 1));
                state.shutter = p->values[16];
                state.particle_spawn_rate = p->values[17];
                state.shutter_decay = p->values[18];
                state.shutter_attack = p->values[19];
                if (p->triggers[0] > 0) {
                    g_received_data.highlight = p->triggers[0];
                }
                if (p->triggers[1] > 0) {
                    g_received_data.blackout = p->triggers[1];
                }
                if (p->triggers[2] > 0) {
                    g_received_data.flash = p->triggers[2];
                }
                if (p->triggers[3] > 0) {
                    g_received_data.random_pixels = p->triggers[3];
                }
                if (p->triggers[4] > 0) {
                    g_received_data.fast_wave = p->triggers[4];
                }
                if (p->triggers[5] > 0) {
                    g_received_data.noise_flash = p->triggers[5];
                }
                if (p->triggers[6] > 0) {
                    g_received_data.segment = p->triggers[6];
                }
                
                xSemaphoreGive(state_mutex);
                // ESP_LOGI(TAG, "Applied fixture payload for fixture_id=%u (index=%d)", FIXTURE_ID, found_idx);
                //ESP_LOGI(TAG, "Triggers: %u %u %u %u %u %u from fixture_id=%u", 
                //    p->triggers[0], p->triggers[1], p->triggers[2], p->triggers[3], p->triggers[4], p->triggers[5], FIXTURE_ID);

                end = esp_timer_get_time();
                receiver_processing_time += (int) end - start;

                

                return;
            } else {
                ESP_LOGW(TAG, "No fixture entry for my fixture_id=%u in broadcast", FIXTURE_ID);
            }
        } else {
            ESP_LOGW(TAG, "Large packet received but failed broadcast validation from %s", mac_str);
        }
        
    }

    // Fallback: legacy format parsing
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
    printf("Device MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

    ESP_LOGI(TAG, "ESP-NOW receiver initialized. Waiting for messages...");
}
 