#include <stdint.h>


typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t r_bg;
    uint8_t g_bg;
    uint8_t b_bg;
    uint8_t noise_level;
    uint8_t D_r;
    uint8_t D_g;
    uint8_t D_b;
    uint8_t v;
    uint8_t f1;
    uint8_t f2;
    uint8_t f3;
} state_t;