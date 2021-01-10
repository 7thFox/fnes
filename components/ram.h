#ifndef RAM_H
#define RAM_H

#include "stdint.h"
#include <stdlib.h>

namespace components
{
class Ram
{
public:
    Ram(uint16_t *addr, uint8_t *data);
    ~Ram();

    void set_clk();

    uint8_t get_byte(uint16_t addr);

private:
    uint8_t ram[0x800];

    uint16_t *address;
    uint8_t *data;

    // uint8_t clk : 1;
};
}
#endif