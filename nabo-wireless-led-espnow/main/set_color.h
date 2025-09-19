#ifndef SET_COLOR_H
#define SET_COLOR_H

#include <stdint.h>
#include <freertos/semphr.h>
#include "led_strip_config.h"
#include "led_pixel.h"
#include "led_strip_config.h"

extern SemaphoreHandle_t led_pixels_mutex;
extern led_pixel_t led_pixels;

void set_color(uint8_t r, uint8_t g, uint8_t b);

void set_background_color(uint8_t r, uint8_t g, uint8_t b);

void set_color_pixel(int index, uint8_t r, uint8_t g, uint8_t b);

void set_color_pixels(int *indexes, uint16_t count, uint8_t r, uint8_t g, uint8_t b);

void set_fast_wave(uint8_t r, uint8_t g, uint8_t b);

void set_noise_flash(uint8_t amplitude);

void hsl_to_rgb(uint8_t H, uint8_t L, uint8_t *r, uint8_t *g, uint8_t *b);


#endif // SET_COLOR_H