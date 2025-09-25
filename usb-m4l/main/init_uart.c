#include "driver/uart.h"


void init_uart(int uart_num, int baud_rate, int buf_size) {
    char* TAG = "uart_usb_receiver";
    
    
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(uart_num, &uart_config);
    /* Allocate a larger driver RX buffer to handle high-rate bursts. */
    uart_driver_install(uart_num, buf_size * 4, 0, 0, NULL, 0);

}