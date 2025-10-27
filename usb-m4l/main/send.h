#ifndef SEND_H
#define SEND_H

#include "message_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


void reset_fixture_triggers(fixture_message_t *fixture);
void reset_all_triggers(broadcast_message_t *message);
int send_broadcast_message(broadcast_message_t *message, const uint8_t *broadcastAddr, SemaphoreHandle_t send_sem);
void init_fixtures(broadcast_message_t *message);

#endif // SEND_H