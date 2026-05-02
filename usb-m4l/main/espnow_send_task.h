#ifndef ESPNOW_SEND_TASK_H
#define ESPNOW_SEND_TASK_H
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "config.h"
#include "message_config.h"


extern fixture_state_t broadcast_message;
extern SemaphoreHandle_t send_sem;

extern int send_task_cycles;
extern int total_send_time;
extern int sent_per_sec;

extern uint8_t broadcastAddr[6];


int send_broadcast_message(esp_message_t *message, const uint8_t *broadcastAddr, SemaphoreHandle_t send_sem);
void espnow_send_task(void *pvParameter);


#endif // ESPNOW_SEND_TASK_H