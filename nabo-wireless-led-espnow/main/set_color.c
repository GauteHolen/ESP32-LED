#include "led_strip_config.h"
#include <stdint.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "led_pixel.h"
#include "set_color.h"
#include "esp_log.h"
#include <math.h>



static const char *TAG = "SET COLOR"; 





void hsl_to_rgb(uint8_t H, uint8_t L, uint8_t *r, uint8_t *g, uint8_t *b) {
    float Hf = (H / 255.0f) * 360.0f;
    float Lf = (L / 255.0f);
    //printf("H: %i, L: %i Hf: %f, Lf: %f\n", H, L, Hf, Lf);
    float C = 1.0f - fabsf(2.0f * Lf - 1.0f);   // chroma
    float X = C * (1.0f - fabsf(fmodf(Hf / 60.0f, 2.0f) - 1.0f));
    float m = Lf - C / 2.0f;

    float r1, g1, b1;

    if (Hf < 60)       { r1 = C; g1 = X; b1 = 0; }
    else if (Hf < 120) { r1 = X; g1 = C; b1 = 0; }
    else if (Hf < 180) { r1 = 0; g1 = C; b1 = X; }
    else if (Hf < 240) { r1 = 0; g1 = X; b1 = C; }
    else if (Hf < 300) { r1 = X; g1 = 0; b1 = C; }
    else              { r1 = C; g1 = 0; b1 = X; }

    *r = (uint8_t)roundf((r1 + m) * 255.0f);
    *g = (uint8_t)roundf((g1 + m) * 255.0f);
    *b = (uint8_t)roundf((b1 + m) * 255.0f);
}



void set_color(uint8_t r, uint8_t g, uint8_t b)
{
    while (1) {
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                led_pixels.r[i] = r;
                led_pixels.g[i] = g;
                led_pixels.b[i] = b;
            }
            xSemaphoreGive(led_pixels_mutex);
            break;
        }
    }
}

void set_background_color(uint8_t r, uint8_t g, uint8_t b)
{
    while (1) {
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                led_pixels.r_bg[i] = r;
                led_pixels.g_bg[i] = g;
                led_pixels.b_bg[i] = b;
            }
            xSemaphoreGive(led_pixels_mutex);
            break;
        }
    }
}

void set_color_pixel(int index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index < 0 || index >= LED_STRIP_LED_COUNT) {
        ESP_LOGW(TAG, "Index out of bounds: %d", index);
        return;
    }
    while (1) {
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            led_pixels.r[index] = r;
            led_pixels.g[index] = g;
            led_pixels.b[index] = b;
            xSemaphoreGive(led_pixels_mutex);
            break;
        }
    }
    
}


void set_color_pixels(int *indexes, uint16_t count, uint8_t r, uint8_t g, uint8_t b)
{
    for (size_t i = 0; i < count; i++) {
        int index = indexes[i];
        if (index < 0 || index >= LED_STRIP_LED_COUNT) {
            ESP_LOGW(TAG, "Index out of bounds: %d", index);
            continue;
        }
        while (1) {
            if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
                led_pixels.r[index] = r;
                led_pixels.g[index] = g;
                led_pixels.b[index] = b;
                xSemaphoreGive(led_pixels_mutex);
                break;
            }
        }
    }
}

void set_fast_wave(uint8_t r, uint8_t g, uint8_t b)
{
    while (1) {
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            
            led_pixels.r_fast[0] = r;
            led_pixels.g_fast[0] = g;
            led_pixels.b_fast[0] = b;

            xSemaphoreGive(led_pixels_mutex);
            break;
        }
    }
}

void set_noise_flash(uint8_t amplitude)
{
    while (1) {
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            led_pixels.noise_amplitude = amplitude;
            xSemaphoreGive(led_pixels_mutex);
            break;
        }
    }
}