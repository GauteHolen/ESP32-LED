#include "config.h"
#include "message_config.h"
#include <stddef.h>
#include "esp_log.h"


void reset_fixture_triggers(fixture_data_t *fixture) {
    if (fixture != NULL) {
        for (int i = 0; i < NUM_TRIGGERS; i++) {
            fixture->triggers[i] = 0;
        }
    }
}

void reset_all_triggers(fixture_state_t *message) {
    if (message != NULL) {
        for (int f = 0; f < NUM_FIXTURES; f++) {
            reset_fixture_triggers(&message->fixtures[f]);
        }
    }
}


void init_fixtures(fixture_state_t *message) {
    for (uint8_t i = 0; i < NUM_FIXTURES; i++) {
        message->fixtures[i].fixture_id = (uint8_t) i + 1;
        message->fixtures[i].num_values_changed = 0;
        for (uint8_t j = 0; j < NUM_VALUES; j++) {
            message->fixtures[i].values[j] = (uint8_t) 0;
            message->fixtures[i].values_changed[j] = false;
            
        }
        message->fixtures[i].num_triggers_changed = 0;
        for (uint8_t j = 0; j < NUM_TRIGGERS; j++) {
            message->fixtures[i].triggers[j] = (uint8_t) 0;
            message->fixtures[i].triggers_changed[j] = false;
        }

    }
}


void reset_fixture_change_flags(fixture_data_t *fixture) {
    if (fixture != NULL) {
        for (int i = 0; i < NUM_VALUES; i++) {
            fixture->values_changed[i] = false;
        }
        fixture->num_values_changed = 0;
        for (int j = 0; j < NUM_TRIGGERS; j++) {
            fixture->triggers_changed[j] = false;
        }
        fixture->num_triggers_changed = 0;
    }
}


void reset_change_flags(fixture_state_t *message) {
    if (message != NULL) {
        for (int f = 0; f < NUM_FIXTURES; f++) {
            reset_fixture_change_flags(&message->fixtures[f]);
        }
    }
}

void count_change_flags(fixture_state_t *message) {
    if (message != NULL) {
        for (int f = 0; f < NUM_FIXTURES; f++) {
            message->fixtures[f].num_values_changed = 0;
            message->fixtures[f].num_triggers_changed = 0;
            for (int v = 0; v < NUM_VALUES; v++) {
                if (message->fixtures[f].values_changed[v]) {
                    message->fixtures[f].num_values_changed++;
                }
            }
            for (int t = 0; t < NUM_TRIGGERS; t++) {
                if (message->fixtures[f].triggers_changed[t]) {
                    message->fixtures[f].num_triggers_changed++;
                }
            }
        }
    }
}


void fixture_data_to_esp_message(fixture_state_t *fixture, esp_message_t *esp_msg){
    if (fixture != NULL && esp_msg != NULL) {
        esp_msg->sequence = fixture->sequence;
        for (int f = 0; f < NUM_FIXTURES; f++) {
            esp_msg->fixtures[f].fixture_id = fixture->fixtures[f].fixture_id;
            for (int v = 0; v < NUM_VALUES; v++) {
                esp_msg->fixtures[f].data.values[v] = fixture->fixtures[f].values[v];
            }
            for (int t = 0; t < NUM_TRIGGERS; t++) {
                esp_msg->fixtures[f].data.triggers[t] = fixture->fixtures[f].triggers[t];
            }
        }
    }
}
