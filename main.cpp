#include "signal.h"
#include "components/rom.h"
#include "components/cpu6502.h"
#include "windows/monitor.h"

int running = 1;
void intHandle(int);

#include <stdlib.h>

int main()
{
    uint8_t prg[100] = {
        0xA9, 0x14,       // LDA 0x14
        0x69, 0x43,       // ADC
        0x4C, 0x40, 0x24, // JMP $4024 (HLT)
    };
    uint8_t image[0xBFE0];
    memcpy(image, prg, 100);
    image[0xFFFC - 0x4020] = 0x4C; // JMP $4020 (start of ROM)
    image[0xFFFD - 0x4020] = 0x40;
    image[0xFFFE - 0x4020] = 0x20;

    uint8_t data_bus;
    uint16_t address_bus;
    Cpu6502 *cpu = cpu_new(&address_bus, &data_bus);
    Rom *rom = rom_new(image, &address_bus, &data_bus);

    uint8_t clk = 0;
    int cycles = 0;
    Monitor *monitor = monitor_init(cpu, rom, &clk, &cycles, &address_bus, &data_bus);

    signal(SIGINT, intHandle);
    cpu_power_on(cpu);
    cpu_resb_(cpu, 0);
    while (running)
    {
        clk = (clk + 1) & 1;

        if (clk == 1)
        {
            cycles++;
        }

        if (cycles == 3)
        {
            cpu_resb_(cpu, 1);
        }
        // clock
        rom_clk(rom, clk);
        cpu_clk(cpu, clk);

        monitor_refresh(monitor);
        monitor_step(monitor);
    }

    monitor_end(monitor);

    free(cpu);
    free(rom);
    free(monitor);
}

void intHandle(int sig)
{
    running = 0;
}