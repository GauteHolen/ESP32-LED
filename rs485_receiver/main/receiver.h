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

void receive_message_task(void);

#endif // RECEIVER_H