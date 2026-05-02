#ifndef RECEIVER_H
#define RECEIVER_H


typedef enum {
    WAIT_SYNC1,
    WAIT_SYNC2,
    READ_LEN,
    READ_SEQ,
    READ_PAYLOAD,
    READ_CRC
} rx_state_t;

typedef struct {
    rx_state_t state;
    uint8_t seq;
    uint8_t len;
    uint8_t idx;
    uint8_t crc;
    uint8_t calc_crc;

    uint8_t payload[256];
} rx_ctx_t;

void receive_message_task(void);

#endif // RECEIVER_H