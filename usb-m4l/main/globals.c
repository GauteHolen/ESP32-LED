#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "globals.h"
#include "esp_log.h"
#include "config.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "uart_usb.h"
#include "utils.h"


static SemaphoreHandle_t messages_mutex;
static SemaphoreHandle_t uart_mutex;
// Semaphore to protect access to new_payload (static binary semaphore)

static StaticSemaphore_t new_payload_sem_buf;
static SemaphoreHandle_t new_payload_sem = NULL;

static SemaphoreHandle_t time_since_change_mutex;



static const char *TAG = "globals";
static const char *TAG_TIMER = "timer since change";

static int time_since_change = 0;
static bool new_payload = false;

uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

fixture_state_t broadcast_message = (fixture_state_t){.sequence = 0};
static fixture_data_t fixtures_buffer[NUM_FIXTURES];


void create_globals() {
    messages_mutex = xSemaphoreCreateMutex();
    if (messages_mutex == NULL) {
        ESP_LOGI(TAG, "Failed to create messages_mutex!");
    }

    uart_mutex = xSemaphoreCreateMutex();
    if (uart_mutex == NULL) {
        ESP_LOGI(TAG, "Failed to create uart_mutex!");
    }

    new_payload_sem = xSemaphoreCreateBinaryStatic(&new_payload_sem_buf);
    if (new_payload_sem == NULL) {
        ESP_LOGW(TAG, "Failed to create new_payload_sem");
    } else {
        // ensure semaphore is in the 'available' state for use as a mutex-like guard
        xSemaphoreGive(new_payload_sem);
    }

    time_since_change_mutex = xSemaphoreCreateMutex();
    if (time_since_change_mutex == NULL) {
        ESP_LOGW(TAG, "Failed to create time_since_change_mutex");
    }
}


void set_time_since_change(int value) {
    if (xSemaphoreTake(time_since_change_mutex, portMAX_DELAY) == pdTRUE) {
        time_since_change = value;
        xSemaphoreGive(time_since_change_mutex);
    } else {
        ESP_LOGW(TAG_TIMER, "Failed to take time_since_change_mutex");
    }
}

void increment_time_since_change(int increment) {
    if (xSemaphoreTake(time_since_change_mutex, portMAX_DELAY) == pdTRUE) {
        time_since_change += increment;
        xSemaphoreGive(time_since_change_mutex);
    } else {
        ESP_LOGW(TAG_TIMER, "Failed to take time_since_change_mutex");
    }
}


int get_time_since_change() {
    int value = 0;
    if (xSemaphoreTake(time_since_change_mutex, portMAX_DELAY) == pdTRUE) {
        value = time_since_change;
        xSemaphoreGive(time_since_change_mutex);
    } else {
        ESP_LOGW(TAG_TIMER, "Failed to take time_since_change_mutex");
    }
    return value;
}

void set_new_payload(bool value) {
    if (xSemaphoreTake(new_payload_sem, portMAX_DELAY) == pdTRUE) {
        new_payload = value;
        xSemaphoreGive(new_payload_sem);
    } else {
        ESP_LOGW(TAG, "Failed to take new_payload_sem");
    }
}

bool get_new_payload() {
    bool value = false;
    if (xSemaphoreTake(new_payload_sem, portMAX_DELAY) == pdTRUE) {
        value = new_payload;
        xSemaphoreGive(new_payload_sem);
    } else {
        ESP_LOGW(TAG, "Failed to take new_payload_sem");
    }
    return value;
}

void take_uart_mutex() {
    if (xSemaphoreTake(uart_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to take uart_mutex");
    }
}

void give_uart_mutex() {
    if (xSemaphoreGive(uart_mutex) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to give uart_mutex");
    }
}

void take_messages_mutex() {
    if (xSemaphoreTake(messages_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to take messages_mutex");
    }
}

void give_messages_mutex() {
    if (xSemaphoreGive(messages_mutex) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to give messages_mutex");
    }
}

void copy_uart_to_broadcast_message() {

    
    // Copy fixtures to buffer under uart_mutex to ensure consistent snapshot of UART data
    take_uart_mutex();
    memcpy(fixtures_buffer, fixtures_uart, sizeof(fixtures_buffer));
    for (int f = 0; f < NUM_FIXTURES; f++) {
        for (int j = 0; j < NUM_TRIGGERS; j++) {
            fixtures_uart[f].triggers[j] = 0;
            fixtures_uart[f].triggers_changed[j] = false;
        }
        reset_fixture_change_flags(&fixtures_uart[f]);
    }
    give_uart_mutex();

    take_messages_mutex();
    // Copy only the payload (values+triggers) to avoid overwriting adjacent memory
    for (int f = 0; f < NUM_FIXTURES; f++) {
        memcpy(&broadcast_message.fixtures[f].values, &fixtures_buffer[f].values, sizeof(fixtures_buffer[f].values));

        for (int v = 0; v < NUM_VALUES; v++) {
            if (fixtures_buffer[f].values_changed[v]) {
                broadcast_message.fixtures[f].values_changed[v] = true;
            }
        }

        for (int t = 0; t < NUM_TRIGGERS; t++) {
            if (fixtures_buffer[f].triggers[t] > 0) {
                // Copy
                broadcast_message.fixtures[f].triggers[t] = fixtures_buffer[f].triggers[t];
            }
            if (fixtures_buffer[f].triggers_changed[t]) {
                broadcast_message.fixtures[f].triggers_changed[t] = true;
            }

        }
    }
    give_messages_mutex();
}