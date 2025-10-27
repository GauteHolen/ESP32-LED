#ifndef INIT_UART_H
#define INIT_UART_H

#include "driver/uart.h"
#include <stdint.h>
#include "message_config.h"

typedef struct {
    int received_per_sec;
    int time_elapsed_received;
    int total_read;
    int read_per_cycle;
    int maxlen;
    int event_count;

} uart_stats;

extern QueueHandle_t uart_queue;
extern fixture_message_t fixtures[NUM_FIXTURES];

void init_uart();
void uart_receive_task(void *pvParameter);




#endif // INIT_UART_H