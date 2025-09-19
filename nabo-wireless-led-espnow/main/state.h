#ifndef STATE_H
#define STATE_H

#include <stdint.h>
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t r_bg;
    uint8_t g_bg;
    uint8_t b_bg;
    uint8_t r_fast;
    uint8_t g_fast;
    uint8_t b_fast;
    uint8_t noise_level;
    float D;
    float trail_amount;
    float trail_decay;
    uint8_t v;
    uint8_t f1;
    uint8_t f2;
    uint8_t f3;
    float flow_amount;
    float decay;
} state_t;

extern volatile state_t state;

#endif // STATE_H

