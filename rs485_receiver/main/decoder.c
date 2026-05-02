#include <stdint.h>
#include "esp_log.h"



static int idx;
uint8_t num_figures_changed;
uint8_t fixture_id;
uint8_t num_triggers_changed;
uint8_t trigger_id;
uint8_t trigger_value;
uint8_t num_values_changed;
uint8_t value_id;
uint8_t value_data;

void decode_message(uint8_t *payload, uint8_t len){
        /*
    [num_fixtures_changed]
    [fixture_id]
    [num_values_changed]
        [index][value]...
    [num_triggers_changed]
        [index][value]...
    */

    num_figures_changed = payload[0];
    idx = 1;

    ESP_LOGI("DECODE", "Number of fixtures changed: %d", num_figures_changed);

    for (int i = 0; i < num_figures_changed; i++) {
        fixture_id = payload[idx++];
        //ESP_LOGI("DECODE", "Processing fixture ID: %d", fixture_id);
        num_values_changed = payload[idx++];
        //ESP_LOGI("DECODE", "Number of values changed: %d", num_values_changed);
        // Process values
        for (int j = 0; j < num_values_changed; j++) {
            value_id = payload[idx++];
            value_data = payload[idx++];
            //ESP_LOGI("DECODE", "Value ID: %d, Value Data: %d", value_id, value_data);
            // Handle value change
        }

        num_triggers_changed = payload[idx++];
        //ESP_LOGI("DECODE", "Fixture ID: %d, Number of triggers changed: %d", fixture_id, num_triggers_changed);
        // Process triggers
        for (int j = 0; j < num_triggers_changed; j++) {
            trigger_id = payload[idx++];
            trigger_value = payload[idx++];
            //ESP_LOGI("DECODE", "Trigger ID: %d, Trigger Value: %d", trigger_id, trigger_value);
            // Handle trigger change
        }
    }



}