
/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "led_strip.h"
#include "esp_log.h"
#include "esp_err.h"
#include <math.h>
#include "esp_timer.h"


#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Set to 1 to use DMA for driving the LED strip, 0 otherwise
// Please note the RMT DMA feature is only available on chips e.g. ESP32-S3/P4
#define LED_STRIP_USE_DMA  0

#if LED_STRIP_USE_DMA
// Numbers of the LED in the strip
#define LED_STRIP_LED_COUNT 256
#define LED_STRIP_MEMORY_BLOCK_WORDS 1024 // this determines the DMA block size
#else
// Numbers of the LED in the strip
#define LED_STRIP_LED_COUNT 300
#define LED_STRIP_MEMORY_BLOCK_WORDS 0 // let the driver choose a proper memory block size automatically
#endif // LED_STRIP_USE_DMA

// GPIO assignment
#define LED_STRIP_GPIO_PIN  18

// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (20 * 1000 * 1000)

static const char *TAG = "example";

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN, // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,      // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,        // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .mem_block_symbols = LED_STRIP_MEMORY_BLOCK_WORDS, // the memory block size used by the RMT channel
        .flags = {
            .with_dma = LED_STRIP_USE_DMA,     // Using DMA can improve performance when driving more LEDs
        }
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
}

#define MAX_LED_STRIP_BRIGHTNESS 100


typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t r_bg;
    uint8_t g_bg;
    uint8_t b_bg;
} led_pixel_t;

bool pulse = true;

led_pixel_t led_pixels[LED_STRIP_LED_COUNT];
SemaphoreHandle_t led_pixels_mutex;
SemaphoreHandle_t pulse_mutex;


static const int refresh_hz = 60;
static const int refresh_sim_hz = 200;


void update_led_pixels(void *pvParameters)
{
    led_strip_handle_t led_strip = (led_strip_handle_t)pvParameters;
    const int wait_time = 1000 / refresh_hz;
    TickType_t last_wake_time = xTaskGetTickCount(); // capture current tick
    while (1) {
        
        // ESP_LOGI(TAG,"update pixels");
        xSemaphoreTake(led_pixels_mutex, portMAX_DELAY);
        
        for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
            led_strip_set_pixel(led_strip, i, 
                MIN(led_pixels[i].r, MAX_LED_STRIP_BRIGHTNESS),
                MIN(led_pixels[i].g, MAX_LED_STRIP_BRIGHTNESS), 
                MIN(led_pixels[i].b, MAX_LED_STRIP_BRIGHTNESS));
        }
        xSemaphoreGive(led_pixels_mutex);
        int64_t start = esp_timer_get_time();
        led_strip_refresh(led_strip);
        int64_t end = esp_timer_get_time();
        printf("\rRefresh took %lld ms, ", (end - start)/1000);
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
        
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
    xSemaphoreGive(led_pixels_mutex);
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
        ESP_LOGW(TAG, "Index out of bounds: %d", index);
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

/*
void hold_color_pixel(int index, int ticks, uint8_t r, uint8_t g, uint8_t b)
{
    if (index < 0 || index >= LED_STRIP_LED_COUNT) {
        ESP_LOGW(TAG, "Index out of bounds: %d", index);
        return;
    }

    int wait_time = 

    while (1) {
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            led_pixels[index].r = r;
            led_pixels[index].g = g;
            led_pixels[index].b = b;
            xSemaphoreGive(led_pixels_mutex);
            break;
        }
    }
    
}*/




void set_color_pixels(int *indexes, size_t count, uint8_t r, uint8_t g, uint8_t b)
{
    for (size_t i = 0; i < count; i++) {
        int index = indexes[i];
        if (index < 0 || index >= LED_STRIP_LED_COUNT) {
            ESP_LOGW(TAG, "Index out of bounds: %d", index);
            continue;
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
}

int rand_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

int max(uint8_t a, uint8_t b){
    return (a > b) ? a : b;
}

int min(uint8_t a, uint8_t b){
    return (a < b) ? a : b;
}

float min_float(float a, float b){
    return (a < b) ? a : b;
}

static float phase[LED_STRIP_LED_COUNT];



void decay_leds(void *pvParameters){
    float lambda = 0.98; // decay factor per step (should be <1 for decay)
    TickType_t last_wake_time = xTaskGetTickCount();
    int wait_time = 1000 / refresh_sim_hz;

    float t = 0.0;
    float f1 = 7.0;
    float f2 = 1.1;
    float f3 = 4.3201;
    float dt = wait_time / 1000.0;
    float dx = 0.016667;
    float dtdx2 = dt / (dx * dx);
    float Dr = 0.5*dtdx2 * 0.005;
    float Dg = 0.5*dtdx2 * 0.005;
    float Db = 0.5*dtdx2 * 0.005;
    float dt2dx = 0.5 * dt / dx;
    float v_max = 0.5*dx/dt;
    printf("v_max: %f\n", v_max);
    printf("D_max: %f\n", dtdx2);
    float v = 0.0*v_max;
    int noise_level = 150;
    int64_t start1, start2, end1, end2;

    // Local buffers for calculations
    static int r[LED_STRIP_LED_COUNT];
    static int g[LED_STRIP_LED_COUNT];
    static int b[LED_STRIP_LED_COUNT];
    static int r_bg[LED_STRIP_LED_COUNT];
    static int g_bg[LED_STRIP_LED_COUNT];
    static int b_bg[LED_STRIP_LED_COUNT];
    float wave;
    int pulse_leds = 10;
    int pulse_counter = 0;
    int pulse_idx;

    // Initialize color pixels
    

    for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
        phase[i] = i * 6.28318 / LED_STRIP_LED_COUNT; // 2 * PI
    }
    int new_bg_r, new_bg_g, new_bg_b;
    while (1) {
        start1 = esp_timer_get_time();

        // Copy led_pixels to local buffers (minimize mutex hold)
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            start2 = esp_timer_get_time();
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                r[i] = led_pixels[i].r;
                g[i] = led_pixels[i].g;
                b[i] = led_pixels[i].b;
                r_bg[i] = led_pixels[i].r_bg;
                g_bg[i] = led_pixels[i].g_bg;
                b_bg[i] = led_pixels[i].b_bg;
            }
            xSemaphoreGive(led_pixels_mutex);
            end2 = esp_timer_get_time();
            //printf("MutexCopy %lldmus ", (end2 - start2));
        }




        // Boundary
        r[0] = r_bg[0];
        g[0] = g_bg[0];
        b[0] = b_bg[0];
        r[LED_STRIP_LED_COUNT-1] = r_bg[LED_STRIP_LED_COUNT-1];
        g[LED_STRIP_LED_COUNT-1] = g_bg[LED_STRIP_LED_COUNT-1];
        b[LED_STRIP_LED_COUNT-1] = b_bg[LED_STRIP_LED_COUNT-1];

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

        for (int i = 1; i < LED_STRIP_LED_COUNT-1; i++) {
            wave = 1.0; //2 + sinf(phase[i] + t * f1) + sinf(phase[i] + t * f2);
            new_bg_r = max(0, r_bg[i]);// * wave / 2 + rand_range(-noise_level, noise_level)*r_bg[i]/255);
            new_bg_g = max(0, g_bg[i]);// * wave / 2 + rand_range(-noise_level, noise_level)*g_bg[i]/255);
            new_bg_b = max(0, b_bg[i]);// * wave / 2 + rand_range(-noise_level, noise_level)*b_bg[i]/255);

            r[i] = r[i] + Dr * dtdx2 *(r[i+1] - 2*r[i] + r[i-1]) - v * dt2dx * (r[i+1] - r[i-1]);
            g[i] = g[i] + Dg * dtdx2 *(g[i+1] - 2*g[i] + g[i-1]) - v * dt2dx * (g[i+1] - g[i-1]);
            b[i] = b[i] + Db * dtdx2 *(b[i+1] - 2*b[i] + b[i-1]) - v * dt2dx * (b[i+1] - b[i-1]);

            r[i] = max(r[i], new_bg_r);
            g[i] = max(g[i], new_bg_g);
            b[i] = max(b[i], new_bg_b);
        }

        // Write results back to led_pixels (minimize mutex hold)
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                led_pixels[i].r = r[i];
                led_pixels[i].g = g[i];
                led_pixels[i].b = b[i];
            }
            xSemaphoreGive(led_pixels_mutex);
        }

        end1 = esp_timer_get_time();
        printf("Loop %lldmus ", (end1 - start1));

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
        t += wait_time / 1000.0;
    }
}

void app_main(void)
{
    printf("FreeRTOS tick rate: %d Hz\n", configTICK_RATE_HZ);

    led_strip_handle_t led_strip = configure_led();
    ESP_ERROR_CHECK(led_strip_clear(led_strip));


    led_pixels_mutex = xSemaphoreCreateMutex();
    pulse_mutex = xSemaphoreCreateMutex();

    if (pulse_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create pulse_mutex");
        return;
    }
    xTaskCreate(update_led_pixels, "UpdateLEDs", 2048, led_strip, 5, NULL);
    xTaskCreate(decay_leds, "DecayLEDs", 2048, NULL, 5, NULL);

    const int wait_time = 100;
    TickType_t last_wake_time = xTaskGetTickCount(); // capture current tick

    ESP_LOGI(TAG, "Pulse test");
    int idx = 0;
    set_color(0,0,0);
    set_background_color(5, 0, 0);
    int rand1 = 0;
    int rand2 = 0;
    int rand3 = 0;

    while (1) {

    }

    while (0) {

        

        rand1 = rand() % LED_STRIP_LED_COUNT;
        set_color_pixel(rand1, 100, 10, 0);

        rand2 = rand() % LED_STRIP_LED_COUNT;
        set_color_pixel(rand2, 50, 0, 10);

        rand3 = rand() % LED_STRIP_LED_COUNT;
        set_color_pixel(rand3, 30, 0, 3);

        idx++; 
        /*set_color_pixel(3,0,100,0);
        set_color_pixel(1,0,100,0);
        set_color_pixel(2,0,100,0);
        */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
    }
}
