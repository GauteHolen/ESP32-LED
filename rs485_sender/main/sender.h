#ifndef SENDER_H
#define SENDER_H

#include <stdint.h>
#include "crc8.h"

#define RS485_MAX_FRAME_LEN 260
#define RS485_MAX_PAYLOAD_LEN (RS485_MAX_FRAME_LEN - 5)

void send_message(const uint8_t *payload, int payload_len, uint8_t seq);

#endif // SENDER_H