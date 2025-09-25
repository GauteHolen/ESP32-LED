

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "espnow_receiver.h"
#include <stdio.h>
#include "freertos/semphr.h"
#include "led_strip.h"
#include "led_strip_config.h"
#include "esp_log.h"
#include "esp_err.h"
#include <math.h>
#include "esp_timer.h"
#include "state.h"
#include "config.h"
#include "utils.h"
#include "set_color.h"
#include "led_pixel.h"
#include "process_pixels.h"
#include "fixture_id_config.h"
#include "esp_timer.h"
#include "driver/gpio.h"




int time_since_received = 0;





bool pulse = true;

led_pixel_t led_pixels;
SemaphoreHandle_t led_pixels_mutex;
SemaphoreHandle_t pulse_mutex;

volatile received_data_t g_received_data;
volatile state_t state;
SemaphoreHandle_t state_mutex;

uint8_t FIXTURE_ID;

int received_per_second;
int receiver_processing_time;



static const char *TAG = "MAIN FILE";


void update_led_pixels(void *pvParameters)
{
    led_strip_handle_t led_strip = (led_strip_handle_t)pvParameters;
    const int wait_time = 1000 / REFRESH_LED_HZ;
    TickType_t last_wake_time = xTaskGetTickCount(); // capture current tick
    while (1) {
        int64_t start = esp_timer_get_time();
        // ESP_LOGI(TAG,"update pixels");
        xSemaphoreTake(led_pixels_mutex, portMAX_DELAY);
        
        for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
            led_strip_set_pixel(led_strip, i, 
                MIN(led_pixels.r[i] + 
                    led_pixels.r_bg[i] + 
                    led_pixels.r_trail[i] + 
                    led_pixels.r_fast_map[i] +
                    led_pixels.r_noise[i], MAX_LED_STRIP_BRIGHTNESS),
                MIN(led_pixels.g[i] + 
                    led_pixels.g_bg[i] + 
                    led_pixels.g_trail[i] + 
                    led_pixels.g_fast_map[i] + 
                    led_pixels.g_noise[i], MAX_LED_STRIP_BRIGHTNESS), 
                MIN(led_pixels.b[i] + 
                    led_pixels.b_bg[i] + 
                    led_pixels.b_trail[i] + 
                    led_pixels.b_fast_map[i] + 
                    led_pixels.b_noise[i], MAX_LED_STRIP_BRIGHTNESS));
        }
        xSemaphoreGive(led_pixels_mutex);
        
        led_strip_refresh(led_strip);
        int64_t end = esp_timer_get_time();
        //printf("\rRefresh took %lld ms, ", (end - start)/1000);
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
        
    }
}





static float phase[LED_STRIP_LED_COUNT];



void decay_leds(void *pvParameters){
    float lambda = 0.98; // decay factor per step (should be <1 for decay)
    TickType_t last_wake_time = xTaskGetTickCount();
    int wait_time = 1000 / REFRESH_SIM_HZ;

    float t = 0.0;
    float freq_coeff = 0.1;
    float f1 = 1.0 * freq_coeff;
    float f2 = 1.0 * freq_coeff;
    float f3 = 1.0 * freq_coeff;

    float dt = wait_time / 1000.0;
    float dx = 0.016667;
    float dtdx2 = dt / (dx * dx);
    float D_coeff = 0.6* (1/256.0);
    float D = 1.0 * D_coeff;
    float trail_amount = 0.0;
    float trail_decay = 0.0;

    float dt2dx = 0.5 * dt / dx;
    float v_max = 0.5*dx/dt;
    float v_coeff = v_max * 0.05;
    printf("v_max: %f\n", v_max);
    printf("D_max: %f\n", dtdx2);
    float v = 0.0*v_max;
    int noise_level = 150;
    int64_t start1, start2, end1, end2;

    // Local buffers for calculations
    static int16_t r[LED_BUFFER_SIZE];
    static int16_t g[LED_BUFFER_SIZE];
    static int16_t b[LED_BUFFER_SIZE];
    static int16_t r_bg[LED_BUFFER_SIZE];
    static int16_t g_bg[LED_BUFFER_SIZE];
    static int16_t b_bg[LED_BUFFER_SIZE];
    static int16_t buffer[LED_BUFFER_SIZE];
    static int16_t r_fast[FAST_WAVE_BUFFER];
    static int16_t g_fast[FAST_WAVE_BUFFER];
    static int16_t b_fast[FAST_WAVE_BUFFER];
    static int16_t buffer_fast[FAST_WAVE_BUFFER];
    static int16_t r_trail[LED_BUFFER_SIZE];
    static int16_t g_trail[LED_BUFFER_SIZE];
    static int16_t b_trail[LED_BUFFER_SIZE];
    static int16_t r_fast_map[LED_BUFFER_SIZE];
    static int16_t g_fast_map[LED_BUFFER_SIZE];
    static int16_t b_fast_map[LED_BUFFER_SIZE];
    static int16_t r_noise_buffer[LED_BUFFER_SIZE];
    static int16_t g_noise_buffer[LED_BUFFER_SIZE];
    static int16_t b_noise_buffer[LED_BUFFER_SIZE];
    static uint8_t random_buffer[LED_BUFFER_SIZE];
    uint8_t r_noise = 0, g_noise = 0, b_noise = 0;

    int noise_flash_counter = 0;
    uint8_t noise_amplitude = 255;

    int pulse_leds = 10;
    int pulse_counter = 0;
    int pulse_idx;
    printf("Fast wave resolution: %d\n fast wave buffer size: %d\n", FAST_WAVE_RESOLUTION, FAST_WAVE_BUFFER);

    float flow_fast_amount = 0.5 / FAST_WAVE_RESOLUTION;

    // Initialize color pixels


    // Precomputed values
    float *phase;
    phase = compute_phase();
    float *sin_computed = (float *)malloc(LED_BUFFER_SIZE * sizeof(float));

    float flow_amount = 1.0;
    uint8_t flow_dir = 1;
    uint8_t update_background_counter = 0;

    while (1) {
        

        // Copy values from state
        if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
            f1 = state.f1 * freq_coeff;
            f2 = state.f2 * freq_coeff;
            f3 = state.f3 * freq_coeff;
            D = state.D * D_coeff;
            trail_amount = state.trail_amount / 255.0;
            trail_decay = state.trail_decay;

            v = state.v * v_coeff / 255.0;
            noise_level = state.noise_level;
            flow_amount = state.flow_amount;
            lambda = state.decay;
            r_noise = state.r_fast;
            g_noise = state.g_fast;
            b_noise = state.b_fast;
            //printf("Flow amount: %f\t Decay: %f Diffusion: %f Trail Decay: %f Trail Amount: %f\r", flow_amount, lambda, D, trail_decay, trail_amount);

            xSemaphoreGive(state_mutex);
        }


        // Copy led_pixels to local buffers (minimize mutex hold)
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            noise_amplitude = led_pixels.noise_amplitude;
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                r[i+BOUNDARY_SIZE] = led_pixels.r[i];
                g[i+BOUNDARY_SIZE] = led_pixels.g[i];
                b[i+BOUNDARY_SIZE] = led_pixels.b[i];
            }
            for (int i = 0; i < FAST_WAVE; i++) {
                r_fast[i+BOUNDARY_SIZE] = led_pixels.r_fast[i];
                g_fast[i+BOUNDARY_SIZE] = led_pixels.g_fast[i];
                b_fast[i+BOUNDARY_SIZE] = led_pixels.b_fast[i];
            }
            if (update_background_counter % BACKGROUND_UPDATE_INTERVAL == 0) {
                //printf("read bg\n");
                for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                    r_bg[i+BOUNDARY_SIZE] = state.r_bg;
                    g_bg[i+BOUNDARY_SIZE] = state.g_bg;
                    b_bg[i+BOUNDARY_SIZE] = state.b_bg;
                }
            }
            xSemaphoreGive(led_pixels_mutex);
            //printf("MutexCopy %lldmus ", (end2 - start2));
        }
        start2 = esp_timer_get_time();
        boundary_conditions(r, LED_BUFFER_SIZE, BOUNDARY_SIZE);
        boundary_conditions(g, LED_BUFFER_SIZE, BOUNDARY_SIZE);
        boundary_conditions(b, LED_BUFFER_SIZE, BOUNDARY_SIZE);
        boundary_conditions_zero(r_fast, FAST_WAVE_BUFFER, BOUNDARY_SIZE);
        boundary_conditions_zero(g_fast, FAST_WAVE_BUFFER, BOUNDARY_SIZE);
        boundary_conditions_zero(b_fast, FAST_WAVE_BUFFER, BOUNDARY_SIZE);
        boundary_conditions_zero(r_fast_map, LED_BUFFER_SIZE, BOUNDARY_SIZE);
        boundary_conditions_zero(g_fast_map, LED_BUFFER_SIZE, BOUNDARY_SIZE);
        boundary_conditions_zero(b_fast_map, LED_BUFFER_SIZE, BOUNDARY_SIZE);
        

        boundary_conditions(r_trail, LED_BUFFER_SIZE, BOUNDARY_SIZE);
        boundary_conditions(g_trail, LED_BUFFER_SIZE, BOUNDARY_SIZE);
        boundary_conditions(b_trail, LED_BUFFER_SIZE, BOUNDARY_SIZE);

        // printf("Noise amplitude: %d\r", noise_amplitude);
        fill_random_bytes(random_buffer, LED_BUFFER_SIZE);
        amplitude_multiply_source(r_noise_buffer, random_buffer, r_noise, noise_amplitude, LED_BUFFER_SIZE);
        amplitude_multiply_source(g_noise_buffer, random_buffer, g_noise, noise_amplitude, LED_BUFFER_SIZE);
        amplitude_multiply_source(b_noise_buffer, random_buffer, b_noise, noise_amplitude, LED_BUFFER_SIZE);
        noise_amplitude = noise_amplitude*lambda;

        flow(r, flow_amount, buffer);
        flow(g, flow_amount, buffer);
        flow(b, flow_amount, buffer);
        diffuse(r, D, buffer);
        diffuse(g, D, buffer);
        diffuse(b, D, buffer);

        start1 = esp_timer_get_time();
        trail(r, r_trail, trail_decay, trail_amount);
        trail(g, g_trail, trail_decay, trail_amount);
        trail(b, b_trail, trail_decay, trail_amount);
        
        end2 = esp_timer_get_time();

        // Flow fast
        flow_fast(r_fast, flow_fast_amount, buffer_fast);
        flow_fast(g_fast, flow_fast_amount, buffer_fast);
        flow_fast(b_fast, flow_fast_amount, buffer_fast);
        map_from_fast_wave(r_fast, r_fast_map);
        map_from_fast_wave(g_fast, g_fast_map);
        map_from_fast_wave(b_fast, b_fast_map);
        trail(r_fast_map, r_trail, trail_decay, trail_amount);
        trail(g_fast_map, g_trail, trail_decay, trail_amount);
        trail(b_fast_map, b_trail, trail_decay, trail_amount);

        decay(r, lambda, LED_BUFFER_SIZE);
        decay(g, lambda, LED_BUFFER_SIZE);
        decay(b, lambda, LED_BUFFER_SIZE);
        decay(r_fast_map, lambda, LED_BUFFER_SIZE);
        decay(g_fast_map, lambda, LED_BUFFER_SIZE);
        decay(b_fast_map, lambda, LED_BUFFER_SIZE);

        clamp_for_uint8(r);
        clamp_for_uint8(g);
        clamp_for_uint8(b);
        clamp_for_uint8(r_trail);
        clamp_for_uint8(g_trail);
        clamp_for_uint8(b_trail);

        
        
        // Background update
        if (update_background_counter % BACKGROUND_UPDATE_INTERVAL == 0) {

            



            if (f1 == 0) {
                //nothing;
            } else{
                // Precompute wave
                compute_waves(phase, t, f1, f2, f3, sin_computed);
                wave(r_bg, sin_computed);
                wave(g_bg, sin_computed);
                wave(b_bg, sin_computed);


                clamp_for_uint8(r_bg);
                clamp_for_uint8(g_bg);
                clamp_for_uint8(b_bg);
            }
        }
            

       
        

        // process_pixels(r,g,b,r_bg,g_bg,b_bg);
        // Write results back to led_pixels (minimize mutex hold)
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            led_pixels.noise_amplitude = noise_amplitude;
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                led_pixels.r[i] = r[i + BOUNDARY_SIZE];
                led_pixels.g[i] = g[i + BOUNDARY_SIZE];
                led_pixels.b[i] = b[i + BOUNDARY_SIZE];
                led_pixels.r_trail[i] = r_trail[i + BOUNDARY_SIZE] * trail_amount;
                led_pixels.g_trail[i] = g_trail[i + BOUNDARY_SIZE] * trail_amount;
                led_pixels.b_trail[i] = b_trail[i + BOUNDARY_SIZE] * trail_amount;
                led_pixels.r_fast_map[i] = r_fast_map[i + BOUNDARY_SIZE];
                led_pixels.g_fast_map[i] = g_fast_map[i + BOUNDARY_SIZE];
                led_pixels.b_fast_map[i] = b_fast_map[i + BOUNDARY_SIZE];
                led_pixels.r_noise[i] = r_noise_buffer[i + BOUNDARY_SIZE];
                led_pixels.g_noise[i] = g_noise_buffer[i + BOUNDARY_SIZE];
                led_pixels.b_noise[i] = b_noise_buffer[i + BOUNDARY_SIZE];
            }
            for (int i = 0; i < FAST_WAVE; i++) {
                led_pixels.r_fast[i] = r_fast[i + BOUNDARY_SIZE];
                led_pixels.g_fast[i] = g_fast[i + BOUNDARY_SIZE];
                led_pixels.b_fast[i] = b_fast[i + BOUNDARY_SIZE];
            }
            if (update_background_counter % BACKGROUND_UPDATE_INTERVAL == 0) {
                //printf("write bg\n");
                for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                    led_pixels.r_bg[i] = r_bg[i + BOUNDARY_SIZE];
                    led_pixels.g_bg[i] = g_bg[i + BOUNDARY_SIZE];
                    led_pixels.b_bg[i] = b_bg[i + BOUNDARY_SIZE];
                }
            }
            xSemaphoreGive(led_pixels_mutex); 
        }

        update_background_counter++;

        end1 = esp_timer_get_time();
        //printf("Inner %lldmus   Whole %lldms\r", (end2 - start2), (end1 - start1) / 1000);
        //printf("Loop %lldmus ", (end1 - start1));

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
    state_mutex = xSemaphoreCreateMutex();

    /* Init ESP-NOW receiver after semaphores are created so the receive
        callback can safely take `state_mutex`/other semaphores. */
    espnow_receiver_init();

    FIXTURE_ID = get_fixture_id_from_mac();
    ESP_LOGI(TAG, "Fixture ID: %d", FIXTURE_ID);
     
	
    // Increase stack sizes: these tasks perform floating-point work, call into
    // SDK libs (printf/led_strip_refresh) and need more stack than 2KB.
    xTaskCreate(update_led_pixels, "UpdateLEDs", 4096, led_strip, 5, NULL);
    xTaskCreate(decay_leds, "DecayLEDs", 8192, NULL, 5, NULL);

   



    const int wait_time = 1000 / REFRESH_RECEIVER_HZ;
    TickType_t last_wake_time = xTaskGetTickCount(); // capture current tick

     // Led pin FLASH
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 1); // LED on
    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(2000));
    gpio_set_level(LED_PIN, 0); // LED off

    ESP_LOGI(TAG, "Pulse test");
    int idx = 0;
    set_color(0,0,0);
    // set_background_color(10, 0, 0);

    int start_time = 0;
    int end_time = 0;
    int time_elapsed = 0;
    

    float flash_mult = 1.0;
    while (1) {
        start_time = esp_timer_get_time();
        

		if (g_received_data.highlight>0) {
			set_color(20, 20, 20);
			g_received_data.highlight = 0;
		}
        if (g_received_data.blackout>0) {
            set_color(0, 0, 0);
            g_received_data.blackout = 0;
        }
        if (g_received_data.flash>0) {
            flash_mult = ((float) g_received_data.flash) / 255.0;
            set_color(state.r * flash_mult, state.g * flash_mult, state.b * flash_mult);
            g_received_data.flash = 0;
        }
        if (g_received_data.random_pixels>0) {
            int num_pixels = g_received_data.random_pixels;
            for (int i = 0; i < num_pixels; i++) {
                set_color_pixel(rand() % LED_STRIP_LED_COUNT, state.r, state.g, state.b);
            }
            g_received_data.random_pixels = 0;
        }
        if (g_received_data.fast_wave>0) {
            flash_mult = ((float) g_received_data.fast_wave) / 255.0;
            set_fast_wave(state.r_fast * flash_mult, state.g_fast * flash_mult, state.b_fast * flash_mult);
            g_received_data.fast_wave = 0;
        }
        if (g_received_data.noise_flash>0) {
            uint8_t amplitude = g_received_data.noise_flash;
            set_noise_flash(amplitude);
            g_received_data.noise_flash = 0;
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
        end_time = esp_timer_get_time();
        time_elapsed += (end_time - start_time);
        if (time_elapsed >= 1000000) {
            ESP_LOGI(TAG, "Received %d packets in the last second. Average processing time: %d us", received_per_second, receiver_processing_time / (received_per_second == 0 ?1 : received_per_second));
            time_elapsed = 0;
            received_per_second = 0;
            receiver_processing_time = 0;
        }

        gpio_set_level(LED_PIN, 0); // LED off
    }
}