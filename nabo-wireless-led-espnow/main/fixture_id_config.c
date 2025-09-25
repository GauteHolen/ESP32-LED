#include "fixture_id_config.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include <string.h>

// Initialize the fixture configuration array
fixture_id_config_t fixture_id_config[NUM_FIXTURES] = {
    {.fixture_id = 1, .mac_adress = {0x64, 0x8F, 0x11, 0x40, 0x02, 0x00}},
    {.fixture_id = 2, .mac_adress = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0x02}},
    {.fixture_id = 3, .mac_adress = {0x88, 0x57, 0x21, 0x68, 0xC7, 0x38}},
    {.fixture_id = 4, .mac_adress = {0x88, 0x57, 0x21, 0x68, 0xE8, 0xC4}},
    {.fixture_id = 5, .mac_adress = {0x84, 0x1F, 0xE8, 0x27, 0xA4, 0x1C}},
    {.fixture_id = 6, .mac_adress = {0x80, 0xF3, 0xDA, 0xAD, 0x89, 0x18}},

    // Add more fixtures as needed
};

static const char *TAG = "FIXTURE_ID";

uint8_t get_fixture_id_from_mac(void) {

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);

    ESP_LOGI(TAG, "Looking up fixture ID for MAC: {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X}",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Loop through all configured fixtures
    for (int i = 0; i < NUM_FIXTURES; i++) {
        // Check if this fixture entry is configured (fixture_id != 0)
        if (fixture_id_config[i].fixture_id != 0) {
            // Compare MAC addresses byte by byte
            if (memcmp(mac, fixture_id_config[i].mac_adress, 6) == 0) {
                ESP_LOGI(TAG, "Found matching fixture ID: %d", fixture_id_config[i].fixture_id); 
                return fixture_id_config[i].fixture_id;
            }
        }
    }
    
    // No matching MAC found
    ESP_LOGW(TAG, "No fixture ID found for this MAC address, using default ID 0");
    return 0; 
}

