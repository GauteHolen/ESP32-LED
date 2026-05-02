#ifndef SENDER_H
#define SENDER_H

#include <stdint.h>
#include "crc8.h"
#include "message_config.h"

#define RS485_MAX_FRAME_LEN 260
#define RS485_MAX_PAYLOAD_LEN (RS485_MAX_FRAME_LEN - 5)

extern int send_task_cycles;
extern int total_send_time;
extern int sent_per_sec;

extern fixture_state_t broadcast_message;

void rs485_send_task(void *pvParameter);

void send_message(const uint8_t *payload, int payload_len, uint8_t seq);

#endif // SENDER_H