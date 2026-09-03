#ifndef UTILS_H
#define UTILS_H
#include "message_config.h"

void reset_fixture_triggers(fixture_data_t *fixture);
void reset_all_triggers(fixture_state_t *message);
void init_fixtures(fixture_state_t *message);
void reset_change_flags(fixture_state_t *message);
void count_change_flags(fixture_state_t *message);
void reset_fixture_change_flags(fixture_data_t *fixture);
void fixture_data_to_esp_message(fixture_state_t *fixture, esp_message_t *esp_msg);
void open_all_shutters(fixture_state_t *message);
void open_shutter(fixture_data_t *fixture);

#endif // UTILS_H