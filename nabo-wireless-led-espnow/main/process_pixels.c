#include "config.h"
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "utils.h"
#include "esp_random.h"
#include <string.h>
#define PI 3.14159265f
#define TWO_PI (2.0f * PI)

static inline float fast_sin(float x) {
    // wrap to [0, 2π]
    x = fmodf(x, TWO_PI);
    if (x < 0) x += TWO_PI;

    if (x > PI) x = TWO_PI - x;  // use symmetry

    return (16.0f * x * (PI - x)) / (5.0f * PI * PI - 4.0f * x * (PI - x));
}

void fill_random_bytes(uint8_t *arr, size_t n) {
    esp_fill_random(arr, n);   // fills with real random bytes from hardware
}

void amplitude_multiply_source(int16_t *c, uint8_t *source, uint8_t color, uint8_t amplitude, int16_t buffer_size)
{
    float factor = color * amplitude / 255.0f;
    for (int i = 0; i < buffer_size; i++) {
        c[i] = (int16_t)(source[i] * factor);
    }
}

void set_0(int16_t *c, int16_t buffer_size)
{
    for (int i = 0; i < buffer_size; i++) {
        c[i] = 0;
    }
}

float* compute_phase(void) {
    static float phase[LED_BUFFER_SIZE];
    static int initialized = 0;

    if (!initialized) {
        for (int i = 0; i < LED_BUFFER_SIZE; i++) {
            phase[i] = i * 6.28318f / LED_BUFFER_SIZE; // 2 * PI
        }
        initialized = 1;
    }

    return phase; // return pointer to static array
}



void process_pixels(int16_t *r, int16_t *g, int16_t *b, int16_t *r_bg, int16_t *g_bg, int16_t *b_bg)
{
    for (int i = 0; i < LED_BUFFER_SIZE; i++) {
        r[i] = r[i];
        g[i] = g[i];
        b[i] = b[i];
    }
}

void boundary_conditions(int16_t *c, int16_t buffer_size, int16_t boundary_size)
{
    for (int i = 0; i < boundary_size; i++) {
        c[i] = c[boundary_size];
        c[buffer_size - boundary_size + i] = c[buffer_size - boundary_size - 1];
    }
}

void boundary_conditions_zero(int16_t *c, int16_t buffer_size, int16_t boundary_size)
{
    for (int i = 0; i < boundary_size; i++) {
        c[i] = 0;
        c[buffer_size - boundary_size + i] = 0;
    }
}

void decay(int16_t *c, float lambda, int16_t buffer_size)
{
    for (int i = 0; i < buffer_size; i++) {
        c[i] = (c[i] * lambda);
    }
}

void trail(int16_t *c, int16_t *c_trail, float decay, float trail_amount)
{
    for (int i = 0; i < LED_BUFFER_SIZE; i++) {
        c_trail[i] = (max(c_trail[i], c[i]) * decay);
    }
}

void diffuse(int16_t *c, float D, int16_t *buffer)
{
    for (int i = 1; i < LED_BUFFER_SIZE - 1; i++) {
        buffer[i] = (c[i] + D * (c[i-1] - 2*c[i] + c[i+1]));
    }
    for (int i = 1; i < LED_BUFFER_SIZE - 1; i++) {
        c[i] = buffer[i];
    }
}

void wave(int16_t *c, float *wave_computed){
    for (int i = 0; i < LED_BUFFER_SIZE; i++) {
        c[i] = max(0, (uint8_t)(c[i] * (0.2 + wave_computed[i])/2.0));
    }
}

void compute_waves(float *phase, float t, float f1, float f2, float f3, float *sin_computed)
{
    if (f2 == 0.0f && f3 == 0.0f) {
        for (int i = 0; i < LED_BUFFER_SIZE; i++) {
            sin_computed[i] = 1  * fast_sin(phase[i] + t * f1);
        }
    } else if (f2 == 0.0f) {
        for (int i = 0; i < LED_BUFFER_SIZE; i++) {
            sin_computed[i] = 0.2+ 1.5 * fast_sin(phase[i] + t * f1) + 0.5 * fast_sin(phase[i] + t * f3);
        }
    } else if (f3==0.0f){
        for (int i = 0; i < LED_BUFFER_SIZE; i++) {
            sin_computed[i] = 1 + 1.5 * fast_sin(phase[i] + t * f1) + fast_sin(phase[i] + t * f2);
        }
    } else {
        for (int i = 0; i < LED_BUFFER_SIZE; i++) {
            sin_computed[i] = 1 + 1.5 * fast_sin(phase[i] + t * f1) + fast_sin(phase[i] + t * f2) + 0.5 * fast_sin(phase[i] + t * f3);
        }
    }
}

void flow_fast(int16_t *c, float flow_amount, int16_t *buffer)
{
    for (int i = 1; i < FAST_WAVE_BUFFER - 1; i++) {
        ;
        buffer[i] =  flow_amount * (c[i+1] - c[i-1]);
    }
    for (int i = 1; i < FAST_WAVE_BUFFER - 1; i++) {
        c[i] = buffer[i];
    }
}

void flow(int16_t *c, float flow_amount, int16_t *buffer)
{
    for (int i = 1; i < LED_BUFFER_SIZE - 1; i++) {
        ;
        buffer[i] =  (1 - abs(flow_amount)) * c[i] + flow_amount * (c[i+1] - c[i-1]);
    }
    for (int i = 1; i < LED_BUFFER_SIZE - 1; i++) {
        c[i] = buffer[i];
    }
}

void zero_channel(int16_t *c, int16_t buffer_size)
{
    for (int i = 0; i < buffer_size; i++) {
        c[i] = 0;
    }
}

void clamp_for_uint8(int16_t *c)
{
    for (int i = 0; i < LED_BUFFER_SIZE; i++) {
        c[i] = (c[i] < 0) ? 0 : (c[i] > 255) ? 255 : c[i];
    }
}

void map_from_fast_wave(int16_t *c_fast, int16_t *c)
{
    for (int i = 0; i < FAST_WAVE; i++) {
        for (int j = 0; j < FAST_WAVE_RESOLUTION; j++) {
            c[i * FAST_WAVE_RESOLUTION + j + BOUNDARY_SIZE] = c_fast[i + BOUNDARY_SIZE]*0.7;
        }
    }
}

/*
    // Boundary neightbour
    r[0] = r[1];
    g[0] = g[1];

        for (int i = 0; i < BOUNDARY_SIZE; i++) {
            r[i] = r[BOUNDARY_SIZE];
            g[i] = g[BOUNDARY_SIZE];
            b[i] = b[BOUNDARY_SIZE];
            r[LED_STRIP_LED_COUNT + BOUNDARY_SIZE + i] = r[LED_STRIP_LED_COUNT + BOUNDARY_SIZE - 1];
        }
        // Boundary neightbour
        r[0] = r[1];
        g[0] = g[1];
        b[0] = b[1];
        r[LED_STRIP_LED_COUNT-1] = r[LED_STRIP_LED_COUNT-2];
        g[LED_STRIP_LED_COUNT-1] = g[LED_STRIP_LED_COUNT-2];
        b[LED_STRIP_LED_COUNT-1] = b[LED_STRIP_LED_COUNT-2];

        

        for (int i = 1; i < LED_STRIP_LED_COUNT-1; i++) {
            wave = 1 + sinf(phase[i] + t * f1);
            r_bg[i] = max(0, r_bg[i] * wave); /// 2 + rand_range(-noise_level, noise_level)*r_bg[i]/255;
            g_bg[i] = max(0, g_bg[i] * wave); // 2 + rand_range(-noise_level, noise_level)*g_bg[i]/255;
            b_bg[i] = max(0, b_bg[i] * wave); // 2 + rand_range(-noise_level, noise_level)*b_bg[i]/255;

            r[i] = r[i] + Dr * dtdx2 *(r[i+1] - 2*r[i] + r[i-1]) - v * dt2dx * (r[i+1] - r[i-1]);
            g[i] = g[i] + Dg * dtdx2 *(g[i+1] - 2*g[i] + g[i-1]) - v * dt2dx * (g[i+1] - g[i-1]);
            b[i] = b[i] + Db * dtdx2 *(b[i+1] - 2*b[i] + b[i-1]) - v * dt2dx * (b[i+1] - b[i-1]);

        }


*/

/*if (xSemaphoreTake(pulse_mutex, portMAX_DELAY) == pdTRUE) {
            if (pulse==true) {
                printf("Pulse LEDs: ");
                for (int i = 0; i < pulse_leds; i++) {
                    r[pulse_counter*pulse_leds+i] = 100;
                }
            }
            xSemaphoreGive(pulse_mutex);
            if (pulse_counter < LED_STRIP_LED_COUNT - pulse_leds) {
                pulse_counter++;
            } else {
                pulse = false;
                pulse_counter = 0;
            }

        }*/