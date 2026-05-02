#ifndef GLOBALS_H
#define GLOBALS_H

#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "message_config.h"




void create_globals();
void set_time_since_change(int value);
void increment_time_since_change(int increment);
int get_time_since_change();
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status);
void set_new_payload(bool value);
bool get_new_payload();
void take_uart_mutex();
void give_uart_mutex();
void take_messages_mutex();
void give_messages_mutex();


void copy_uart_to_broadcast_message();

#endif // GLOBALS_H