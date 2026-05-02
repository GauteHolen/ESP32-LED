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



int parse_byte(rx_ctx_t *ctx, uint8_t b)
{
    switch (ctx->state) {

    case WAIT_SYNC1:
        if (b == SYNC1)
            ctx->state = WAIT_SYNC2;
        break;

    case WAIT_SYNC2:
        if (b == SYNC2)
            ctx->state = READ_LEN;
        else
            ctx->state = WAIT_SYNC1;
        break;

    case READ_LEN:
        ctx->len = b;

        if (ctx->len < 1 || ctx->len > sizeof(ctx->payload) + 1) {
            ctx->state = WAIT_SYNC1;
            break;
        }

        ctx->idx = 0;
        ctx->state = READ_SEQ;
    break;

    case READ_SEQ:
        ctx->seq = b;
        ctx->state = READ_PAYLOAD;
        break;

    case READ_PAYLOAD:
        ctx->payload[ctx->idx++] = b;

        if (ctx->idx >= ctx->len - 1) {
            ctx->state = READ_CRC;
        }
    break;

    case READ_CRC:
        ctx->crc = b;

        {
            uint8_t tmp[257];
            tmp[0] = ctx->seq;
            memcpy(&tmp[1], ctx->payload, ctx->len - 1);

            ctx->calc_crc = crc8(tmp, ctx->len);

            ctx->state = WAIT_SYNC1;

            if (ctx->calc_crc == ctx->crc) {
                return 1;   // valid frame
            } else {
                return -1;  // CRC fail
            }
        }
    }

    return 0; // still parsing
}


void receive_message_task(void)
{
    uint8_t data[256];
    rx_ctx_t ctx = {0};

    while (1) {

        int rlen = uart_read_bytes(UART_PORT, data, sizeof(data), pdMS_TO_TICKS(20));

        for (int i = 0; i < rlen; i++) {
            int res = parse_byte(&ctx, data[i]);

            if (res == 1) {
                ESP_LOGI("RX", "Valid frame seq=%d len=%d", ctx.seq, ctx.len);
                decode_message(ctx.payload, ctx.len);
            }
            else if (res == -1) {
                ESP_LOGE("RX", "CRC error");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
