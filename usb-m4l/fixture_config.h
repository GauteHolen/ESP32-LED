#ifndef FIXTURE_CONFIG_H
#define FIXTURE_CONFIG_H

#include <stdint.h>
#include "esp_message_config.h"


#define NUM_FIXTURES 16

typedef struct {
    uint8_t values[NUM_VALUES];
    uint8_t triggers[NUM_TRIGGERS];
} payload;


#endif // FIXTURE_CONFIG_H