#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <stdbool.h>


#define NUM_VALUES 30
#define NUM_TRIGGERS 15
#define NUM_FIXTURES 18




typedef struct {
    uint8_t values[NUM_VALUES];
    bool values_changed[NUM_VALUES];
} values_t;

typedef struct {
    uint8_t triggers[NUM_TRIGGERS];
    bool triggers_changed[NUM_TRIGGERS];
} triggers_t;


typedef struct {
    uint8_t fixture_id;
    uint8_t num_triggers_changed;
    triggers_t trigger_data;
    uint8_t num_values_changed;
    values_t value_data;
} fixture_data_t;

typedef struct  
{
    fixture_data_t fixtures[NUM_FIXTURES];
    /* data */
} fixtures_t;

#endif // MESSAGE_H