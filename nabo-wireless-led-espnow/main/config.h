#ifndef CONFIG_H
#define CONFIG_H

#include "led_strip_config.h"

// Brightness control
#define MAX_LED_STRIP_BRIGHTNESS 50

// Simulation & refresh rates
#define REFRESH_RECEIVER_HZ 40
#define REFRESH_MESSAGES_HZ 100
#define REFRESH_SIM_HZ 100
#define BACKGROUND_UPDATE_INTERVAL 3
#define REFRESH_LED_HZ 60
#define FAST_WAVE_RESOLUTION 10
#define FAST_WAVE (LED_STRIP_LED_COUNT / FAST_WAVE_RESOLUTION)
#define FAST_WAVE_BUFFER (FAST_WAVE + 2 * BOUNDARY_SIZE)


// Buffer layout
#define BOUNDARY_SIZE 2
#define LED_BUFFER_SIZE (LED_STRIP_LED_COUNT + 2 * BOUNDARY_SIZE)


// Particle system configuration
#define PARTICLE_BUFFER_SIZE 250
#define PARTICLE_RESOLUTION 10
#define PARTICLE_GRID_SIZE (LED_STRIP_LED_COUNT * PARTICLE_RESOLUTION)

// Other project-wide constants can go here

#endif // CONFIG_H
