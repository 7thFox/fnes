#include "rom.h"

namespace components
{
    Rom::Rom(uint8_t *image, uint16_t *addr, uint8_t *data)
    {
        this->image = image;
        this->address = addr;
        this->data = data;
    }

    Rom::~Rom() {}

    void Rom::set_clk(int sig)
    {
        uint8_t newClk = sig & 1;
        if (newClk == 1 && this->clk == 0) // Rising-edge
        {
            if (*this->address >= 0x4020)
            {
                (*this->data) = this->image[(*this->address) - 0x4020];
            }
        }
        this->clk = newClk;
    }

    uint8_t Rom::debug_get_at_addr(uint16_t addr, char *str_out)
    {
        // if (addr >= 0x4020 && addr <= 0xFFFF)
        // {
        //     auto op = this->image[addr - 0x4020];
        //     auto meta = Cpu6502::metadata[op];
        //     switch (meta->addr_mode)
        //     {
        //     case AddressMode::imp:
        //         break;
        //     }
        // }
        return 0;
    }
}