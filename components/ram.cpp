#include "ram.h"

namespace components
{
Ram::Ram(uint16_t *addr, uint8_t *data)
{
    this->address = addr;
    this->data = data;
}

Ram::~Ram() {}

void Ram::set_clk()
{
    if (*this->address < 0x0800)// TODO: mirroring
    {
        (*this->data) = this->get_byte(*this->address);
    }
}

uint8_t Ram::get_byte(uint16_t addr){

    if (addr < 0x0800)// TODO: mirroring
    {
        return this->ram[addr];
    }
    return 0x00;
}

}