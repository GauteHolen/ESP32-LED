#ifndef INIT_UART_H
#define INIT_UART_H

#include "driver/uart.h"

void init_uart(int uart_num, int baud_rate, int buf_size);

#endif // INIT_UART_H