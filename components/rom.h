#ifndef ROM_H
#define ROM_H

#include "stdint.h"
#include <stdlib.h>

typedef struct rom_s Rom;

struct rom_s
{
    uint8_t *image;

    uint16_t *address;
    uint8_t *data;

    uint8_t clk : 1;
};

Rom *rom_new(uint8_t *image, uint16_t *addr, uint8_t *data);
void rom_clk(Rom *rom, int sig);

#endif