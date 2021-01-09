#include "cpu6502.h"

void _cpu_clk_resb(Cpu6502 *cpu);
void _cpu_clk_rising(Cpu6502 *cpu);
void _cpu_clk_falling(Cpu6502 *cpu);

Cpu6502 *cpu_new(uint16_t *addr, uint8_t *data)
{
    Cpu6502 *cpu = (Cpu6502 *)malloc(sizeof(Cpu6502));
    cpu->address = addr;
    cpu->data = data;
    return cpu;
}

void cpu_clk(Cpu6502 *cpu, int sig)
{
    uint8_t clk_next = sig & 1;
    if (cpu->resb_ == 1)
    {
        int falling = cpu->clk == 1 && clk_next == 0;

        if (cpu->emu_cycles > 0)
        {
            if (falling) // rising-edge (and not emulating cycles taken)
            {
                cpu->emu_cycles--;
            }
        }
        else if (falling)
        {
            _cpu_clk_falling(cpu);
        }
        else
        {

            _cpu_clk_rising(cpu);
        }
    }
    cpu->clk = clk_next;
}
void _cpu_clk_rising(Cpu6502 *cpu)
{
    cpu->inst[0] = *cpu->data;
}

void _cpu_clk_falling(Cpu6502 *cpu)
{
    (*cpu->address) = cpu->pc;
}

void cpu_power_on(Cpu6502 *cpu)
{
    cpu->emu_cycles = 0;

    cpu->p = 0x34;
    cpu->a = cpu->x = cpu->y = 0x00;
    cpu->s = 0xfd;
}

void cpu_resb_(Cpu6502 *cpu, int sig)
{
    cpu->resb_ = sig & 1;
    if (cpu->resb_)
    {
        cpu->s -= 3;
        cpu->pc = 0xFFFC;
        //cpu->irq = true;
    }
}