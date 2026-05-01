#include <string.h>
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_private/esp_clk.h"
#include "esp_log.h"
#include "message_config.h"
#include "setup_uart.h"
#include "receiver.h"
#include "crc8.h"
#include "decoder.h"


void receive_message_task(void)
{
    uint8_t data[64];

    static rx_state_t state = WAIT_SYNC1;

    static uint8_t len = 0;
    static uint8_t seq = 0; 
    static uint8_t payload[128];
    static int idx = 0;
    static uint8_t crc_recv = 0;
    static uint8_t crc_calc;

    while (1) {

        int rlen = uart_read_bytes(UART_PORT, data, sizeof(data), pdMS_TO_TICKS(20));



        for (int i = 0; i < rlen; i++) {

            uint8_t b = data[i];

            switch (state) {

            case WAIT_SYNC1:
                if (b == SYNC1)
                    state = WAIT_SYNC2;
                break;

            case WAIT_SYNC2:
                if (b == SYNC2)
                    state = READ_LEN;
                else
                    state = WAIT_SYNC1;
                break;

            case READ_LEN:
                len = b;
                idx = 0;
                state = READ_SEQ;
                break;

            case READ_SEQ:
                if (seq == 0) {
                    ESP_LOGI("RECV", "START OF NEW SEQUENCE");
                } 

                if (b == (uint8_t)(seq + 1)) {
                    // expected sequence number
                } else {
                    ESP_LOGE("RECV", "SEQ ERROR expected=%d got=%d\n", (uint8_t)(seq + 1), b);
                }
                seq = b;
                state = READ_PAYLOAD;
                break;

            case READ_PAYLOAD:
                payload[idx++] = b;

                if (idx >= len - 1) {
                    state = READ_CRC;
                }
                break;

            case READ_CRC:
                crc_recv = b;

                // ---- verify CRC ----
                {
                    uint8_t tmp[128];
                    tmp[0] = seq;
                    memcpy(&tmp[1], payload, len - 1);

                    crc_calc = crc8(tmp, len);

                    if (crc_calc == crc_recv) {
                        // printf("FRAME OK seq=%d payload=", seq);
                        decode_message(payload, len - 1);
                        // printf("\n");
                    } else {
                        ESP_LOGE("RECV", "CRC ERROR seq=%d\n", seq);
                    }
                }

                state = WAIT_SYNC1;
                break;
            }
        }
    }
}
