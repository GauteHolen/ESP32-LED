
#include <stdint.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int rand_range(int min, int max) {
    return min + rand() % (max - min + 1);
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