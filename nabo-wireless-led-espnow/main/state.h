#ifndef STATE_H
#define STATE_H

#include <stdint.h>

/**
 *  @brief Global state struct that holds all the parameters for the LED effects.
 *  This struct is updated by the ESP-NOW receive callback and read by the main LED processing loop.
 *  It includes parameters for colors, effect intensities, decay rates, and other effect-specific settings.
 *
 *  @param r Base red color (0-255)
 *  @param g Base green color (0-255)
 *  @param b Base blue color (0-255)
 *  @param r_bg Background red color (0-255)
 *  @param g_bg Background green color (0-255)
 *  @param b_bg Background blue color (0-255)
 *  @param r_fast Fast wave red color (0-255)
 *  @param g_fast Fast wave green color (0-255)
 *  @param b_fast Fast wave blue color (0-255)
 *  @param noise_level Intensity of noise effect (0-255)
 *  @param D Diffusion coefficient for pixel spreading
 *  @param trail_amount Amount of trailing effect (0.0-1.0)
 *  @param trail_decay Decay rate for the trailing effect (0.0-1.0)
 *  @param v Velocity for pixel movement (0.0-1.0)
 *  @param f1 Frequency 1 for wave effects
 *  @param f2 Frequency 2 for wave effects
 *  @param f3 Frequency 3 for wave effects
 *  @param flow_amount Amount of flow effect (-1.0 to 1.0)
 *  @param decay Overall decay rate for pixel brightness (0.0-1.0)
 *  @param shutter Shutter effect parameter (0-255)
 *  @param particle_spawn_rate Rate at which particles are spawned (0-255)
 *  @param shutter_decay Decay rate for shutter effect (0-255)
 *  @param shutter_attack Attack rate for shutter effect (0-255)
 */
typedef struct {
    uint8_t r; 
    uint8_t g;
    uint8_t b;
    uint8_t r_bg;
    uint8_t g_bg;
    uint8_t b_bg;
    uint8_t r_fast;
    uint8_t g_fast;
    uint8_t b_fast;
    uint8_t noise_level;
    float D;
    float trail_amount;
    float trail_decay;
    int8_t v;
    uint8_t f1;
    uint8_t f2;
    uint8_t f3;
    float flow_amount;
    float decay;
    uint8_t shutter;
    uint8_t particle_spawn_rate;
    uint8_t shutter_decay;
    uint8_t shutter_attack;

    uint8_t r_particles;
    uint8_t g_particles;
    uint8_t b_particles;
    float particle_decay;
    int8_t particle_velocity;
    uint8_t particle_life;
    float particle_velocity_decay;
    float particle_color_decay;
    uint8_t particle_spawn_start;
    uint8_t particle_spawn_end;
    uint8_t particle_static_velocity;


} state_t;

extern volatile state_t state;

#endif // STATE_H

