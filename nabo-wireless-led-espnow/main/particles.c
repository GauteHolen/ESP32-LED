#include "config.h"
#include "particles.h"
#include "esp_log.h"
#include "utils.h"

static const char *TAG = "PARTICLES";

void init_particles(volatile particle_t *particles){
    for (int i = 0; i < PARTICLE_BUFFER_SIZE; i++) {
        particles->active[i] = false;
        particles->position[i] = 0;
        particles->last_position[i] = 0;
        particles->velocity[i] = 0;
        particles->velocity_decay[i] = 1.0f;
        particles->color_decay[i] = 1.0f;
        particles->r[i] = 0;
        particles->g[i] = 0;
        particles->b[i] = 0;
        particles->life[i] = 0;
    }
}

void spawn_particle(volatile particle_t *particles, uint8_t r, uint8_t g, uint8_t b, uint16_t life, int16_t position, int16_t last_position, int8_t velocity, float velocity_decay, float color_decay){
    //ESP_LOGI(TAG, "Spawning particle at position %d; life %d; velocity %d", position, life, velocity);
    for (int i = 0; i < PARTICLE_BUFFER_SIZE; i++) {
        if (!particles->active[i]) {
            particles->active[i] = true;
            particles->position[i] = position;
            particles->last_position[i] = last_position;
            particles->velocity[i] = velocity;
            particles->r[i] = r;
            particles->g[i] = g;
            particles->b[i] = b;
            particles->life[i] = life*5*PARTICLE_RESOLUTION; // Scale life by resolution to make it more intuitive
            particles->velocity_decay[i] = velocity_decay;
            particles->color_decay[i] = color_decay;
            break; // Spawn one particle at a time
        }
    }
}

void update_particles(volatile particle_t *particles){
    for (int i = 0; i < PARTICLE_BUFFER_SIZE; i++) { 
        if (particles->active[i]) {
            // Move particle according to velocity
            particles->last_position[i] = particles->position[i];
            particles->position[i] += particles->velocity[i];

            // Decrease life
            if (particles->life[i] > 0) {
                particles->life[i]--;
            }

            // Apply decay to velocity and color (optional, can be tweaked for different effects)
            /**if (particles->velocity > 0) {
                particles->velocity[i] = max(1, (int8_t)(particles->velocity[i] * particles->velocity_decay[i]));
            }
            else if (particles->velocity < 0) {
                particles->velocity[i] = -max(1, (int8_t)(-particles->velocity[i] * particles->velocity_decay[i]));
            }
            */
           particles->velocity[i] = (int8_t)(particles->velocity[i] * particles->velocity_decay[i]);

            particles->r[i] = (uint8_t)(particles->r[i] * particles->color_decay[i]);
            particles->g[i] = (uint8_t)(particles->g[i] * particles->color_decay[i]);
            particles->b[i] = (uint8_t)(particles->b[i] * particles->color_decay[i]);

            // Deactivate if life is zero or out of bounds
            if (particles->life[i] == 0 || particles->position[i] < 0 || particles->position[i] >= PARTICLE_GRID_SIZE || particles->velocity[i] == 0) {
                particles->active[i] = false;
            }
        }
    }
}

void render_particles(volatile particle_t *particles, int16_t *r_buffer, int16_t *g_buffer, int16_t *b_buffer){
    for (int i = 0; i < PARTICLE_BUFFER_SIZE; i++) {
        if (particles->active[i]) {
            int16_t pos = particles->position[i] / PARTICLE_RESOLUTION; // Account for boundary offset
            int16_t last_pos = particles->last_position[i] / PARTICLE_RESOLUTION;
            int16_t diff = pos - last_pos;

            if (diff > 0){
                for (int16_t p = last_pos; p <= pos; p++) {
                    if (p >= 0 && p < LED_STRIP_LED_COUNT) {
                        r_buffer[p] += particles->r[i];
                        g_buffer[p] += particles->g[i];
                        b_buffer[p] += particles->b[i];
                    }
                }
            }
            else if (diff < 0){
                for (int16_t p = last_pos; p >= pos; p--) {
                    if (p >= 0 && p < LED_STRIP_LED_COUNT) {
                        r_buffer[p] += particles->r[i];
                        g_buffer[p] += particles->g[i];
                        b_buffer[p] += particles->b[i];
                    }
                }
            }

            if (pos >= 0 && pos < LED_STRIP_LED_COUNT) {
                r_buffer[pos] += particles->r[i];
                g_buffer[pos] += particles->g[i];
                b_buffer[pos] += particles->b[i];
            }
        }
    }
}