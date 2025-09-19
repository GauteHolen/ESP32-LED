#ifndef ESPNOW_RECEIVER_H
#define ESPNOW_RECEIVER_H

#include <stdint.h>
#include "esp_now.h"

#ifdef __cplusplus
extern "C" {
#endif

// Callback for received ESP-NOW data
void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);

// Initialize Wi-Fi, ESP-NOW, and register the receive callback
void espnow_receiver_init(void);

#ifdef __cplusplus
}
#endif

#endif // ESPNOW_RECEIVER_H
