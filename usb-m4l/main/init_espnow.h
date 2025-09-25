#ifndef INIT_ESPNOW_H
#define INIT_ESPNOW_H

#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

void init_espnow(uint8_t *broadcastAddr);

#endif // INIT_ESPNOW_H