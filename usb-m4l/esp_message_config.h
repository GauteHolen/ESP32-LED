#ifndef ESP_MESSAGE_CONFIG_H
#define ESP_MESSAGE_CONFIG_H

#include "fixture_config.h"

#define NUM_VALUES 20
#define NUM_TRIGGERS 10

typedef struct {
    uint8_t fixture_id;
    uint8_t mac[6];
    payload data;
} esp_message_t;

#endif // ESP_MESSAGE_CONFIG_H
