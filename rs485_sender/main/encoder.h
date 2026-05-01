#ifndef ENCODER_H
#define ENCODER_H

#include "message_config.h"
#include <stdint.h>

int encode_message(fixtures_t *data, uint8_t *buffer, int buffer_size);

#endif // ENCODER_H

