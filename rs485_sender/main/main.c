#include <string.h>
#include "setup_uart.h"
#include "sender.h"
#include <stdint.h>
#include "esp_log.h"
#include "encoder.h"
#include "message_config.h"

#define BUFFER_SIZE 1024
static uint8_t tx_buffer[BUFFER_SIZE];
static fixtures_t payload = {0};


void app_main(void)
{
    setup_uart();

    //const char *payload = "HELLO FROM ESP! HOPE YOU RECEIVE THIS MESSAGE CORRECTLY.";

    for (int i = 0; i < NUM_FIXTURES; i++) {
        payload.fixtures[i].fixture_id = i;
    }

    payload.fixtures[0].num_values_changed = 2;
    payload.fixtures[0].value_data.values_changed[3] = true;
    payload.fixtures[0].value_data.values[3] = 123;
    payload.fixtures[0].value_data.values_changed[7] = true;
    payload.fixtures[0].value_data.values[7] = 45;

    payload.fixtures[3].num_values_changed = 1;
    payload.fixtures[3].value_data.values_changed[6] = true;
    payload.fixtures[3].value_data.values[6] = 69;

    payload.fixtures[7].num_triggers_changed = 1;
    payload.fixtures[7].trigger_data.triggers_changed[2] = true;
    payload.fixtures[7].trigger_data.triggers[2] = 1;



    uint8_t seq = 0;

    while (1) {
        if (seq==0) {
            ESP_LOGI("UART", "START OF NEW SEQUENCE");
        }

        int message_length = encode_message(&payload, tx_buffer, BUFFER_SIZE);
        send_message(tx_buffer, message_length, seq++);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}