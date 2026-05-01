#include "setup_uart.h"
#include "sender.h"
#include "crc8.h"
#include <string.h>
#include <stdint.h>
#include "esp_log.h"

static const char *TAG = "SENDER";

uint8_t frame[RS485_MAX_FRAME_LEN];
uint8_t len_pos;
uint8_t crc;

void send_message(const uint8_t *payload, int payload_len, uint8_t seq)
{
    if (payload == NULL || payload_len < 0) {
        ESP_LOGW(TAG, "Invalid payload");
        return;
    }

    if (payload_len > RS485_MAX_PAYLOAD_LEN) {
        ESP_LOGW(TAG, "Payload too large (%d > %d), dropping frame", payload_len, RS485_MAX_PAYLOAD_LEN);
        return;
    }

    
    int i = 0;

    // ---- sync word ----
    frame[i++] = SYNC1;
    frame[i++] = SYNC2;

    // ---- length (SEQ + DATA) ----
    len_pos = i++;
    
    // ---- sequence ----
    frame[i++] = seq;

    // ---- payload ----
    memcpy(&frame[i], payload, payload_len);
    i += payload_len;

    // ---- fill length ----
    frame[len_pos] = (i - 3); // exclude sync + len byte

    // ---- CRC over (SEQ + DATA) ----
    crc = crc8(&frame[3], i - 3);
    frame[i++] = crc;

    // ---- send ----
    uart_write_bytes(UART_PORT, (const char *)frame, i);

    //ESP_LOGI("UART", "Sent framed packet seq=%d", seq - 1);
}