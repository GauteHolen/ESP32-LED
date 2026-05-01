#include "driver/uart.h"

#define UART_PORT UART_NUM_2
#define TXD_PIN 17
#define RXD_PIN 16


uint8_t data[128];

while (1) {
    int len = uart_read_bytes(UART_NUM_2, data, sizeof(data)-1, pdMS_TO_TICKS(100));
    if (len > 0) {
        data[len] = 0;
        printf("Received: %s\n", data);
    }
}