#ifndef PARTICLES_H
#define PARTICLES_H
#include <stdint.h>
#include "config.h"
/**
 * @brief Struct representing a particle in the particle system. Each particle has a position, velocity, color, and life.
 * The particle system will be updated each frame to move particles according to their velocity, apply decay to their life, and remove them when life reaches zero. The main LED processing loop will read the active particles and render them on the LED strip.
 * @param active Indicates whether the particle is currently active (true) or inactive (false).
 * @param position The current position of the particle on the LED strip (0 to LED_STRIP_LED_COUNT-1).
 * @param last_position The previous position of the particle, used for rendering trails or movement effects.
 * @param velocity The velocity of the particle, which determines how it moves across the LED strip each frame. Positive values move from the start to the end of the strip, negative values move from the end to the start.
 * @param r The red color component of the particle (0-255).
 * @param g The green color component of the particle (0-255).
 * @param b The blue color component of the particle (0-255).
 * @param life The remaining life of the particle, which decreases each frame. When life reaches zero, the particle becomes inactive and is removed from the simulation.
 */
typedef struct {
    bool active[PARTICLE_BUFFER_SIZE];
    int16_t position[PARTICLE_BUFFER_SIZE];
    int16_t last_position[PARTICLE_BUFFER_SIZE];
    int8_t velocity[PARTICLE_BUFFER_SIZE];
    uint8_t r[PARTICLE_BUFFER_SIZE];
    uint8_t g[PARTICLE_BUFFER_SIZE];
    uint8_t b[PARTICLE_BUFFER_SIZE];
    uint16_t life[PARTICLE_BUFFER_SIZE];
    float velocity_decay[PARTICLE_BUFFER_SIZE];
    float color_decay[PARTICLE_BUFFER_SIZE];
} particle_t;

extern volatile particle_t particles;

void init_particles(volatile particle_t *particles);

void spawn_particle(volatile particle_t *particles, uint8_t r, uint8_t g, uint8_t b, uint16_t life, int16_t position, int16_t last_position, int8_t velocity, float velocity_decay, float color_decay);

void update_particles(volatile particle_t *particles);

void render_particles(volatile particle_t *particles, int16_t *r_buffer, int16_t *g_buffer, int16_t *b_buffer);

#endif 