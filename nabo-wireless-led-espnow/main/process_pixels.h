#ifndef PROCESS_PIXELS_H
#define PROCESS_PIXELS_H


#include <stdint.h>
#include <stdio.h>
#include <stddef.h>




void process_pixels(int16_t *r, int16_t *g, int16_t *b, int16_t *r_bg, int16_t *g_bg, int16_t *b_bg);
void boundary_conditions(int16_t *c, int16_t buffer_size, int16_t boundary_size);
void boundary_conditions_zero(int16_t *c, int16_t buffer_size, int16_t boundary_size);
void zero_channel(int16_t *c, int16_t buffer_size);
void decay(int16_t *c, float lambda, int16_t buffer_size);
void wave(int16_t *c, float *wave_computed);
float* compute_phase(void);
void compute_waves(float *phase, float t, float f1, float f2, float f3, float *sin_computed);
void flow(int16_t *c, float flow_amount, int16_t *buffer);
void flow_fast(int16_t *c, float flow_amount, int16_t *buffer);
void map_from_fast_wave(int16_t *c_fast, int16_t *c);
void clamp_for_uint8(int16_t *c);
void diffuse(int16_t *c, float D, int16_t *buffer);
void trail(int16_t *c, int16_t *c_trail, float decay, float trail_amount);
void fill_random_bytes(uint8_t *arr, size_t n);
void amplitude_multiply_source(int16_t *c, uint8_t *source, uint8_t color, uint8_t amplitude, int16_t buffer_size);


#endif // PROCESS_PIXELS_H