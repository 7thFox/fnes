#include "cpu6502.h"

Cpu6502::Cpu6502(uint16_t *addr, uint8_t *data)
{
    this->address = addr;
    this->data = data;
    this->vcc = 0;
    this->state = new _Cpu6502_state(std::bind(&Cpu6502::state_power_off, this));
}
Cpu6502::~Cpu6502()
{
    delete this->state;
}

void Cpu6502::set_clk(int sig)
{
    uint8_t clk_next = sig & 1;
    if (this->clk != clk_next)
    {
        this->clk = clk_next;
        auto next = (*this->state)();
        delete this->state;
        this->state = next;
    }
    this->clk = clk_next;
}

void Cpu6502::power_on()
{
    this->emu_cycles = 0;

    this->p = 0x34;
    this->a = this->x = this->y = 0x00;
    this->s = 0xfd;
    this->vcc = 1;
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

_Cpu6502_state *Cpu6502::state_wait_cycles(int ncycles, _Cpu6502_state *continue_with)
{
    std::cout << "state_wait_cycles " << ncycles << " " << (int)this->clk << std::endl;
    if (this->clk == 1)
    {
        if (ncycles == 1)
        {
            return continue_with;
        }
        return new _Cpu6502_state(std::bind(&Cpu6502::state_wait_cycles, this, ncycles - 1, continue_with));
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::state_wait_cycles, this, ncycles, continue_with));
}
// states
_Cpu6502_state *Cpu6502::state_power_off()
{
    std::cout << "state_power_off" << std::endl;
    if (this->vcc)
    {
        return new _Cpu6502_state(std::bind(&Cpu6502::state_power_on_dirty, this));
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::state_power_off, this));
}

_Cpu6502_state *Cpu6502::state_power_on_dirty()
{
    std::cout << "state_power_on_dirty" << std::endl;
    if (!this->resb_)
    {
        return new _Cpu6502_state(std::bind(&Cpu6502::state_reset_hold, this));
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::state_power_on_dirty, this));
}
_Cpu6502_state *Cpu6502::state_reset_hold()
{
    std::cout << "state_reset_hold" << std::endl;
    if (this->resb_)
    {
        auto cnt = new _Cpu6502_state(std::bind(&Cpu6502::state_begin_fetch, this));
        return new _Cpu6502_state(std::bind(&Cpu6502::state_wait_cycles, this, 2, cnt));
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::state_reset_hold, this));
}

_Cpu6502_state *Cpu6502::state_begin_fetch()
{
    std::cout << "state_begin_fetch" << std::endl;
    if (this->clk == 0)
    {
        (*this->address) = this->pc;
        return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_first_inst_byte, this));
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::state_begin_fetch, this));
}

_Cpu6502_state *Cpu6502::state_fetch_first_inst_byte()
{
    std::cout << "state_fetch_first_inst_byte" << std::endl;
    if (this->clk == 1)
    {
        this->inst[0] = *this->data;
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_first_inst_byte, this));
}

_Cpu6502_state *Cpu6502::state_hlt()
{
    std::cout << "state_hlt" << std::endl;
    return new _Cpu6502_state(std::bind(&Cpu6502::state_hlt, this));
}
