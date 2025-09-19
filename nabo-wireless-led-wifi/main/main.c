#include <esp_timer.h>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "freertos/semphr.h"
#include "led_strip.h"
#include "led_strip_config.h"
#include "esp_log.h"
#include "esp_err.h"
#include <math.h>
#include "trigger_events.h"
#include "state.h"




volatile state_t state;
SemaphoreHandle_t state_mutex;

#define WIFI_SSID      "thewifi24Ghz"
#define WIFI_PASS      "thewifipassword"


void wifi_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("Connecting to WiFi...\n");
    ESP_ERROR_CHECK(esp_wifi_connect());

    int retry = 0;
    wifi_ap_record_t ap_info;
    while (retry < 100) {
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            printf("Connected to WiFi!\n");
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        retry++;
    }
    if (retry == 100) {
        printf("WiFi connection timeout!\n");
    }
}

#define REFRESH_WIFI_HZ 100;

void udp_server_task(void *pvParameters)
{
    TickType_t last_wake_time = xTaskGetTickCount(); // capture current tick
    const int wait_time = 1000 / REFRESH_WIFI_HZ;
    const int port = 8000;
    int sock;
    struct sockaddr_in server_addr;
    struct sockaddr_storage source_addr;
    socklen_t socklen = sizeof(source_addr);
    char rx_buffer[128];

    // Create UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        printf("Unable to create socket: errno %d\n", errno);
        vTaskDelete(NULL);
        return;
    }

    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Bind socket to port
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Socket unable to bind: errno %d\n", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    printf("UDP server listening on port %d\n", port);

    while (1) {
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0,
                          (struct sockaddr *)&source_addr, &socklen);
        if (len < 0) {
            printf("recvfrom failed: errno %d\n", errno);
            break;
        } else if (len >= 32) {
            // OSC decode: /nabo, ,iiii, 4 int32 args
            const uint8_t *data = (const uint8_t *)rx_buffer;
            // Parse address (should be "/nabo")
            if (memcmp(data, "/nabo", 5) == 0) {
                // Find end of address (null-terminated, padded to 4 bytes)
                int addr_end = 0;
                while (addr_end < len && data[addr_end] != 0) addr_end++;
                addr_end = (addr_end + 4) & ~3; // pad to next multiple of 4
                // Parse type tag (should be ,iiii)
                if (memcmp(&data[addr_end], ",iiii", 5) == 0) {
                    int tag_end = addr_end;
                    while (tag_end < len && data[tag_end] != 0) tag_end++;
                    tag_end = (tag_end + 4) & ~3;
                    // Now parse 4 int32s (big-endian)
                    if (tag_end + 16 <= len) {
                        uint8_t out[4];
                        for (int i = 0; i < 4; ++i) {
                            int32_t val = (data[tag_end + i*4] << 24) |
                                          (data[tag_end + i*4 + 1] << 16) |
                                          (data[tag_end + i*4 + 2] << 8) |
                                          (data[tag_end + i*4 + 3]);
                            out[i] = (uint8_t)val;
                        }
                        // out[2] = control, out[3] = value
                        uint8_t control = out[2];
                        uint8_t value = out[3];
                        printf("Received control %d value %d\n", control, value);
                        if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
                            switch (control) {
                                case 0:
                                    state.r = value;
                                    break;
                                case 1:
                                    state.g = value;
                                    break;
                                case 2:
                                    state.b = value;
                                    break;
                                case 3:
                                    state.r_bg = value;
                                    set_background_color(state.r_bg, state.g_bg, state.b_bg);
                                    break;
                                case 4:
                                    state.g_bg = value;
                                    set_background_color(state.r_bg, state.g_bg, state.b_bg);
                                    break;
                                case 5:
                                    state.b_bg = value;
                                    set_background_color(state.r_bg, state.g_bg, state.b_bg);
                                    break;
                                case 6:
                                    state.f1 = value;
                                    break;
                                case 7:
                                    state.f2 = value;
                                    break;
                                case 8:
                                    state.f3 = value;
                                    break;
                                case 9:
                                    state.D_r = value;
                                    break;
                                case 10:
                                    state.D_g = value;
                                    break;
                                case 11:
                                    state.D_b = value;
                                    break;
                                case 12:
                                    state.v = value - 127;
                                    break;
                                case 13:
                                    state.noise_level = value;
                                    break;
                                case 20:
                                    blackout();
                                    break;
                                case 21:
                                    highlight(value);
                                    break;
                                case 22:                                
                                    random_pixels(value);          
                                    break;
                                case 23:
                                    set_color(state.r, state.g, state.b);
                                    break;
                                default:
                                    break;
                            }
                            xSemaphoreGive(state_mutex);
                        }
                    }
                }
            }
        }
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
    }

    if (sock != -1) {
        printf("Shutting down socket...\n");
        close(sock);
    }
    vTaskDelete(NULL);
}

// Definition of global received data struct and mutex








#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))



#define MAX_LED_STRIP_BRIGHTNESS 100
#define REFRESH_RECEIVER_HZ 100



bool pulse = true;

led_pixel_t led_pixels[LED_STRIP_LED_COUNT];
SemaphoreHandle_t led_pixels_mutex;



static const int refresh_hz = 60;
static const int refresh_sim_hz = 100;
static const char *TAG = "WIFI_RECV";


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
                MIN(led_pixels[i].r + led_pixels[i].r_bg, MAX_LED_STRIP_BRIGHTNESS),
                MIN(led_pixels[i].g + led_pixels[i].g_bg, MAX_LED_STRIP_BRIGHTNESS),
                MIN(led_pixels[i].b + led_pixels[i].b_bg, MAX_LED_STRIP_BRIGHTNESS));
        }
        xSemaphoreGive(led_pixels_mutex);
        int64_t start = esp_timer_get_time();
        led_strip_refresh(led_strip);
        int64_t end = esp_timer_get_time();
        printf("\rRefresh took %lld ms, ", (end - start)/1000);
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
        
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
    float freq_coeff = 0.1;
    float f1 = 1.0 * freq_coeff;
    float f2 = 1.0 * freq_coeff;
    float f3 = 1.0 * freq_coeff;

    float dt = wait_time / 1000.0;
    float dx = 0.016667;
    float dtdx2 = dt / (dx * dx);
    float D_coeff = 0.5 * dtdx2 * 0.0005;
    float Dr = 1.0 * D_coeff;
    float Dg = 1.0 * D_coeff;
    float Db = 1.0 * D_coeff;
    float dt2dx = 0.5 * dt / dx;
    float v_max = 0.5*dx/dt;
    float v_coeff = v_max * 0.05;
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
    while (1) {
        //start1 = esp_timer_get_time();

        // Copy values from state
        if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
            f1 = state.f1 * freq_coeff;
            f2 = state.f2 * freq_coeff;
            f3 = state.f3 * freq_coeff;
            Dr = state.D_r * D_coeff;
            Dg = state.D_g * D_coeff;
            Db = state.D_b * D_coeff;
            v = state.v * v_coeff / 255.0;
            noise_level = state.noise_level;

            xSemaphoreGive(state_mutex);
        }


        // Copy led_pixels to local buffers (minimize mutex hold)
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            start2 = esp_timer_get_time();
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                r[i] = led_pixels[i].r;
                g[i] = led_pixels[i].g;
                b[i] = led_pixels[i].b;
                r_bg[i] = state.r_bg;
                g_bg[i] = state.g_bg;
                b_bg[i] = state.b_bg;
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
            wave = 1 + sinf(phase[i] + t);
            r_bg[i] = max(0, r_bg[i]) * wave; /// 2 + rand_range(-noise_level, noise_level)*r_bg[i]/255;
            g_bg[i] = max(0, g_bg[i]) * wave; // 2 + rand_range(-noise_level, noise_level)*g_bg[i]/255;
            b_bg[i] = max(0, b_bg[i]) * wave; // 2 + rand_range(-noise_level, noise_level)*b_bg[i]/255;

            r[i] = r[i] + Dr * dtdx2 *(r[i+1] - 2*r[i] + r[i-1]) - v * dt2dx * (r[i+1] - r[i-1]);
            g[i] = g[i] + Dg * dtdx2 *(g[i+1] - 2*g[i] + g[i-1]) - v * dt2dx * (g[i+1] - g[i-1]);
            b[i] = b[i] + Db * dtdx2 *(b[i+1] - 2*b[i] + b[i-1]) - v * dt2dx * (b[i+1] - b[i-1]);

        }

        // Write results back to led_pixels (minimize mutex hold)
        if (xSemaphoreTake(led_pixels_mutex, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                led_pixels[i].r = r[i];
                led_pixels[i].g = g[i];
                led_pixels[i].b = b[i];
                led_pixels[i].r_bg = r_bg[i];
                led_pixels[i].g_bg = g_bg[i];
                led_pixels[i].b_bg = b_bg[i];
            }
            xSemaphoreGive(led_pixels_mutex);
        }

        //end1 = esp_timer_get_time();
        //printf("Loop %lldmus ", (end1 - start1));

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(wait_time));
        t += wait_time / 1000.0;
    }
}

void app_main(void)
{
    state_mutex = xSemaphoreCreateMutex();

    printf("FreeRTOS tick rate: %d Hz\n", configTICK_RATE_HZ);

    led_strip_handle_t led_strip = configure_led();
    ESP_ERROR_CHECK(led_strip_clear(led_strip));

    led_pixels_mutex = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(update_led_pixels, "UpdateLEDs", 4096, led_strip, 5, NULL, 0); // Core 0
    xTaskCreatePinnedToCore(decay_leds, "DecayLEDs", 2048, NULL, 3, NULL, 0); // Core 0

    wifi_init();
    xTaskCreatePinnedToCore(udp_server_task, "udp_server", 4096, NULL, 3, NULL, 1); // Core 1
    set_background_color(0,0,10);

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}