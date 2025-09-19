#ifndef UTILS_H
#define UTILS_H


#include <stdint.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int rand_range(int min, int max);
int max(uint8_t a, uint8_t b);
int min(uint8_t a, uint8_t b);
float min_float(float a, float b);

#endif // UTILS_H