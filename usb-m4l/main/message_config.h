#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>


#define NUM_VALUES 30
#define NUM_TRIGGERS 15
#define NUM_FIXTURES 18


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
} broadcast_message_t;

#endif // MESSAGE_H