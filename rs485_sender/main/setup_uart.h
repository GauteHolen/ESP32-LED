#ifndef SETUP_UART_H
#define SETUP_UART_H

#include "driver/uart.h"
#include "esp_system.h"
#include "esp_private/esp_clk.h"
#include "esp_log.h"
#define UART_PORT UART_NUM_2
#define TXD_PIN 17
#define RXD_PIN 16

#define SYNC1 0xAA
#define SYNC2 0x55

void setup_uart(void);

#endif // SETUP_UART_H