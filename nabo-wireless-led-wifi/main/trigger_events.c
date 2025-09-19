#include "led_strip_config.h"
#include "esp_log.h"
#include "trigger_events.h"
#include "led_strip.h"
#include "freertos/semphr.h"
#include <stdint.h>
#include "state.h"

extern led_pixel_t led_pixels[LED_STRIP_LED_COUNT];
extern SemaphoreHandle_t led_pixels_mutex;
extern state_t state;
extern SemaphoreHandle_t state_mutex;


void update_background_color(){
    while (1) {
        if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
            set_background_color(state.r_bg, state.g_bg, state.b_bg);
            xSemaphoreGive(state_mutex);
            break;
        }
    }
}


void set_color(uint8_t r, uint8_t g, uint8_t b)
{
    while (1) {
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                led_pixels[i].r = r;
                led_pixels[i].g = g;
                led_pixels[i].b = b;
            }
            xSemaphoreGive(led_pixels_mutex);
            break;
        }
    }
}

void highlight(uint8_t dim) {
    set_color(dim, dim, dim);
}

void blackout(void) {
    set_color(0, 0, 0);
}



void set_background_color(uint8_t r, uint8_t g, uint8_t b)
{
    while (1) {
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                led_pixels[i].r_bg = r;
                led_pixels[i].g_bg = g;
                led_pixels[i].b_bg = b;
            }
            xSemaphoreGive(led_pixels_mutex);
            break;
        }
    }
    xSemaphoreGive(led_pixels_mutex);
}

void set_color_pixel(int index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index < 0 || index >= LED_STRIP_LED_COUNT) {
        ESP_LOGW("LED Strip", "Index out of bounds: %d", index);
        return;
    }
    while (1) {
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            led_pixels[index].r = r;
            led_pixels[index].g = g;
            led_pixels[index].b = b;
            xSemaphoreGive(led_pixels_mutex);
            break;
        }
    }
    
}

int rand_range(int min, int max) {
    return (min + rand() % (max - min + 1));
}

void random_pixels(uint8_t count){
    while (1) {
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < count; i++) {
                int index = rand_range(0, LED_STRIP_LED_COUNT - 1);
                led_pixels[index].r = state.r;
                led_pixels[index].g = state.g;
                led_pixels[index].b = state.b;
            }
            xSemaphoreGive(led_pixels_mutex);
            break;
        }
    }
}
