#ifndef ROM_H
#define ROM_H

#include "stdint.h"
#include <stdlib.h>
#include "cpu6502.h"

namespace components
{
class Rom
{
public:
    Rom(uint8_t *image, uint16_t *addr, uint8_t *data);
    ~Rom();

    void set_clk();

    uint8_t debug_get_at_addr(uint16_t addr, char* strbytes, char* strdis);

private:
    uint8_t *image;

    uint16_t *address;
    uint8_t *data;

    // uint8_t clk : 1;
};
}
#endif