#ifndef TRIGGER_EVENTS_H
#define TRIGGER_EVENTS_H

#include <stdint.h>

void set_color(uint8_t r, uint8_t g, uint8_t b);
void highlight(uint8_t dim);
void blackout(void);
void update_background_color(void);
void set_background_color(uint8_t r, uint8_t g, uint8_t b);
void set_color_pixel(int index, uint8_t r, uint8_t g, uint8_t b);
void random_pixels(uint8_t count);
int rand_range(int min, int max);


#endif // TRIGGER_EVENTS_H
