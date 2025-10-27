#ifndef ESPNOW_RECEIVER_H
#define ESPNOW_RECEIVER_H

#include <stdint.h>
#include <freertos/semphr.h>
#include "esp_now.h"
#include "state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LED_PIN 2

// Extern declaration for global received data struct
typedef struct {
	uint8_t highlight;
	uint8_t blackout;
	uint8_t flash;
	uint8_t random_pixels;
	uint8_t fast_wave;
	uint8_t noise_flash;
	uint8_t segment;
} received_data_t;


extern uint8_t FIXTURE_ID;
extern volatile received_data_t g_received_data;
extern volatile state_t state;
extern SemaphoreHandle_t state_mutex;
extern int received_per_second;
extern int receiver_processing_time;


// Callback for received ESP-NOW data
void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);

// Initialize Wi-Fi, ESP-NOW, and register the receive callback
void espnow_receiver_init(void);

#ifdef __cplusplus
}
#endif

#endif // ESPNOW_RECEIVER_H
