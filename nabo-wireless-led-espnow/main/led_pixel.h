#ifndef LED_PIXEL_H
#define LED_PIXEL_H

#include <stdint.h>
#include "led_strip_config.h"
#include "config.h"

typedef struct {
    uint8_t r[LED_STRIP_LED_COUNT];
    uint8_t g[LED_STRIP_LED_COUNT];
    uint8_t b[LED_STRIP_LED_COUNT];
    uint8_t r_bg[LED_STRIP_LED_COUNT];
    uint8_t g_bg[LED_STRIP_LED_COUNT];
    uint8_t b_bg[LED_STRIP_LED_COUNT];
    uint8_t r_trail[LED_STRIP_LED_COUNT];
    uint8_t g_trail[LED_STRIP_LED_COUNT];
    uint8_t b_trail[LED_STRIP_LED_COUNT];
    uint8_t r_fast[FAST_WAVE];
    uint8_t g_fast[FAST_WAVE];
    uint8_t b_fast[FAST_WAVE];
    uint8_t r_fast_map[LED_STRIP_LED_COUNT];
    uint8_t g_fast_map[LED_STRIP_LED_COUNT];
    uint8_t b_fast_map[LED_STRIP_LED_COUNT];
    uint8_t noise_amplitude;
    uint8_t r_noise[LED_BUFFER_SIZE];
    uint8_t g_noise[LED_BUFFER_SIZE];
    uint8_t b_noise[LED_BUFFER_SIZE];
} led_pixel_t;


extern led_pixel_t led_pixels;

#endif // LED_PIXEL_H