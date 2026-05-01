#include "driver/uart.h"

#define UART_PORT UART_NUM_2
#define TXD_PIN 17
#define RXD_PIN 16

void app_main(void)
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_PORT, 1024, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    const char *msg = "Hello from ESP32\n";

    while (1) {
        uart_write_bytes(UART_PORT, msg, strlen(msg));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}