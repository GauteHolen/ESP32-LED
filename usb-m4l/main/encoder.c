#include "message_config.h"
#include <stdint.h>

int idx;
uint8_t num_fixtures_changed;


int encode_message(fixture_state_t *data, uint8_t *buffer, int buffer_size){
    /*
    [num_fixtures_changed]
    [fixture_id]
    [num_values_changed]
        [index][value]...
    [num_triggers_changed]
        [index][value]...
    */
    num_fixtures_changed = 0;

    idx = 1; // First byte reserved for num_fixtures_changed
    
    for (int i = 0; i < NUM_FIXTURES; i++) {

        if (data->fixtures[i].num_triggers_changed > 0 || data->fixtures[i].num_values_changed > 0) {
            // ---- fixture id ----
            buffer[idx++] = data->fixtures[i].fixture_id;

            // ---- num values changed ----
            buffer[idx++] = data->fixtures[i].num_values_changed;

            // ---- values (index + value) ----
            for (int v = 0; v < NUM_VALUES; v++) {
                if (data->fixtures[i].values_changed[v]) {
                    buffer[idx++] = v; // index
                    buffer[idx++] = data->fixtures[i].values[v]; // value
                }
            }

            // ---- num triggers changed ----
            buffer[idx++] = data->fixtures[i].num_triggers_changed;

            // ---- triggers (index + value) ----
            for (int t = 0; t < NUM_TRIGGERS; t++) {
                if (data->fixtures[i].triggers_changed[t]) {
                    buffer[idx++] = t; // index
                    buffer[idx++] = data->fixtures[i].triggers[t]; // value
                }
            }

            num_fixtures_changed++;
        }
    }
    buffer[0] = num_fixtures_changed;
    return idx;
}
