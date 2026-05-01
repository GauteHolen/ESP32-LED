#include "stdint.h"

uint8_t crc8(const uint8_t *data, int len)
{
    uint8_t crc = 0x00;

    for (int i = 0; i < len; i++) {
        crc ^= data[i];

        for (int b = 0; b < 8; b++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }

    return crc;
}