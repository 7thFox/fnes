#include "cpu6502.h"

Cpu6502::Cpu6502(uint16_t *addr, uint8_t *data)
{
    this->address = addr;
    this->data = data;
}
Cpu6502::~Cpu6502() {}

void Cpu6502::set_clk(int sig)
{
    uint8_t clk_next = sig & 1;
    if (this->resb_ == 1)
    {
        int falling = this->clk == 1 && clk_next == 0;

        if (this->emu_cycles > 0)
        {
            if (falling) // rising-edge (and not emulating cycles taken)
            {
                this->emu_cycles--;
            }
        }
        else if (falling)
        {
            this->clk_falling();
        }
        else
        {

            this->clk_rising();
        }
    }
    this->clk = clk_next;
}
void Cpu6502::clk_rising()
{
    this->inst[0] = *this->data;
}

void Cpu6502::clk_falling()
{
    (*this->address) = this->pc;
}

void Cpu6502::power_on()
{
    this->emu_cycles = 0;

    this->p = 0x34;
    this->a = this->x = this->y = 0x00;
    this->s = 0xfd;
}

void Cpu6502::set_resb_(int sig)
{
    this->resb_ = sig & 1;
    if (this->resb_)
    {
        this->s -= 3;
        this->pc = 0xFFFC;
        //cpu->irq = true;
    }
}

uint8_t Cpu6502::get_a() { return this->a; }
uint8_t Cpu6502::get_x() { return this->x; }
uint8_t Cpu6502::get_y() { return this->y; }
uint8_t Cpu6502::get_p() { return this->p; }
uint8_t Cpu6502::get_s() { return this->s; }
uint16_t Cpu6502::get_pc() { return this->pc; }