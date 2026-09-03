#include "driver/uart.h"
#include <stdio.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "uart_usb.h"
#include "message_config.h"
#include "globals.h"
#include "utils.h"


fixture_data_t fixtures_uart[NUM_FIXTURES] = {0}; // Define the fixtures array here
QueueHandle_t uart_queue = NULL;
const int buf_size = 8192;
const int uart_num = UART_NUM_0;
const int baud_rate = 115200; // 921600 bps; ensure host/CH340 support this
 // larger buffer to absorb bursts
const int tx_pin = UART_PIN_NO_CHANGE; // Use default TX pin
const int rx_pin = UART_PIN_NO_CHANGE; // Use default RX pin
static const char *TAG = "uart_usb";

void init_uart() {
    const char *TAG = "init_uart";
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Install driver with event queue (queue size 20)
    uart_driver_install(uart_num, buf_size, 0, 20, &uart_queue, 0);
    uart_set_rx_full_threshold(uart_num, 1);
    uart_set_rx_timeout(uart_num, 1);

    ESP_LOGI(TAG, "UART initialized");
}


void uart_receive_task(void *pvParameter) {

    uart_stats *stats = (uart_stats *)pvParameter;

    const char *TAG = "uart_receive_task";
    ESP_LOGI(TAG,  "Listening for serial messages...");
    int buf_idx = 0;

    int64_t start, end;
    int start_read, end_read;

    int len, total, i;
    uint8_t control;
    uint16_t value;
    uint8_t fixture_id;
    /* initialize fixtures buffer to zero to avoid copying uninitialized memory */

    
    uint8_t *data = malloc(buf_size);
    uart_event_t event;
   
    

    for (;;) {
        if (xQueueReceive(uart_queue, &event, portMAX_DELAY)) {
            
            if (event.type == UART_DATA) {
                stats->event_count++;
                
                /* shorter timeout to reduce per-call latency; read more often */
                len = uart_read_bytes(uart_num, data + buf_idx, buf_size - buf_idx, 0);
                if (len > stats->maxlen) {
                    stats->maxlen = len;
                }
                if (len > 0) {
                    total = buf_idx + len;
                    i = 0;
                    start_read = esp_timer_get_time();
                    take_uart_mutex();
                    while (i <= total - 5) {
                        if (data[i] == 255) {
                            control = data[i+1];
                            value = (data[i+2] << 8) | data[i+3];
                            fixture_id = data[i+4];
                            //printf("Fixture ID: %d, Control: %u, Value: %u\n", fixture_id, control, value);
                            

                            // validate fixture_id before indexing
                            if (fixture_id < 1 || fixture_id > NUM_FIXTURES) {
                                
                                ESP_LOGW(TAG, "Received out-of-range fixture_id: %d", fixture_id);
                            } else {
                                int idx = fixture_id - 1; 
                                stats->received_per_sec++;
                                
                                if (control < NUM_VALUES) {
                                    fixtures_uart[idx].values[control] = (uint8_t)value;
                                    fixtures_uart[idx].values_changed[control] = true;
                                }
                                else if (control >= NUM_VALUES && control < (NUM_VALUES + NUM_TRIGGERS)) {
                                    fixtures_uart[idx].triggers[control - NUM_VALUES] = (uint8_t)value;
                                    fixtures_uart[idx].triggers_changed[control - NUM_VALUES] = true;
                                    // ESP_LOGI(TAG, "Received trigger change for fixture %d, trigger %d: %d", fixture_id, control - NUM_VALUES, value);
                                }
                            }
                            

                            i += 5;
                        } else {
                            i++;
                        }
                    }

                    give_uart_mutex();
                    // Move any leftover bytes to the start of the buffer
                    buf_idx = total - i;
                    if (buf_idx > 0 && i < total) {
                        memmove(data, data + i, buf_idx);
                    }
                    end_read = esp_timer_get_time();
                    stats->total_read += end_read - start_read;                   
                    
                    //vTaskDelay(1); // allow copy and broadcast of updated data and
                    stats->read_per_cycle++;
                }                
            }
        }
    }
}