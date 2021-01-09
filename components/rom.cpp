#include "rom.h"

Rom *rom_new(uint8_t *image, uint16_t *addr, uint8_t *data)
{
    Rom *rom = (Rom *)malloc(sizeof(Rom));
    rom->image = image;
    rom->address = addr;
    rom->data = data;
    return rom;
}

void rom_clk(Rom *rom, int sig)
{
    uint8_t newClk = sig & 1;
    if (newClk == 1 && rom->clk == 0) // Rising-edge
    {
        if (*rom->address > 0x4020)
        {
            (*rom->data) = rom->image[(*rom->address) - 0x4020];
        }
    }
    rom->clk = newClk;
}