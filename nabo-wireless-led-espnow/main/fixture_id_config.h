#ifndef FIXTURE_ID_CONFIG_H
#define FIXTURE_ID_CONFIG_H
#include <stdint.h>

#define NUM_FIXTURES 16

typedef struct {
    uint8_t fixture_id;
    uint8_t mac_adress[6];

} fixture_id_config_t;


extern fixture_id_config_t fixture_id_config[NUM_FIXTURES];


// Make fixture id mac address map

uint8_t get_fixture_id_from_mac(void);
#endif // FIXTURE_ID_CONFIG_H