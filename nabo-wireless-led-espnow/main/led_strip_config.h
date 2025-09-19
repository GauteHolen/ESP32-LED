#ifndef LED_STRIP_CONFIG_H
#define LED_STRIP_CONFIG_H

#include "led_strip.h"

// Set to 1 to use DMA for driving the LED strip, 0 otherwise
#define LED_STRIP_USE_DMA  0

#if LED_STRIP_USE_DMA
#define LED_STRIP_LED_COUNT 256
#define LED_STRIP_MEMORY_BLOCK_WORDS 1024
#else
#define LED_STRIP_LED_COUNT 300
#define LED_STRIP_MEMORY_BLOCK_WORDS 0
#endif

#define LED_STRIP_GPIO_PIN  23
#define LED_STRIP_RMT_RES_HZ  (20 * 1000 * 1000)

led_strip_handle_t configure_led(void);

#endif // LED_STRIP_CONFIG_H
