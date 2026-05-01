#include "driver/uart.h"
#include "esp_system.h"
#include "esp_private/esp_clk.h"
#include "esp_log.h"
#include "setup_uart.h"

void setup_uart(void)
{    
    uart_config_t uart_config = {
        .baud_rate = 230400,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_PORT, 2048, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_mode(UART_PORT, UART_MODE_RS485_HALF_DUPLEX);
}