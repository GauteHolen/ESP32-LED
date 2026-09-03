#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <stdbool.h>


#define NUM_VALUES 30
#define NUM_TRIGGERS 15
#define NUM_FIXTURES 52
#define MAGIC_ESPNOW {'N','A','B','O'}


typedef struct {
    uint8_t fixture_id;
    uint8_t num_triggers_changed;
    uint8_t triggers[NUM_TRIGGERS];
    bool triggers_changed[NUM_TRIGGERS];
    uint8_t num_values_changed;
    uint8_t values[NUM_VALUES];
    bool values_changed[NUM_VALUES];
} fixture_data_t;



typedef struct {
    uint8_t sequence;              // increment each frame
    fixture_data_t fixtures[NUM_FIXTURES];
} fixture_state_t;

typedef struct {
    uint8_t values[NUM_VALUES];
    uint8_t triggers[NUM_TRIGGERS];
} payload;


typedef struct {
    uint8_t fixture_id;
    payload data;
} fixture_message_t;

typedef struct {
    uint8_t magic[4];                 // e.g. 0xAB, sanity check
    uint8_t sequence;              // increment each frame
    fixture_message_t fixtures[NUM_FIXTURES];
} esp_message_t;

#endif // MESSAGE_H