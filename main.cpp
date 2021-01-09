#include "signal.h"
#include "components/rom.h"
#include "components/cpu6502.h"
#include "windows/monitor.h"

int running = 1;
void intHandle(int);

#include <stdlib.h>
#include "unistd.h"
#include <iostream>

//#define DEBUG_OUT_NO_NCURSES

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
    Cpu6502 *cpu = new Cpu6502(&address_bus, &data_bus);
    Rom *rom = new Rom(image, &address_bus, &data_bus);

    uint8_t clk = 0;
    int cycles = 0;
#ifndef DEBUG_OUT_NO_NCURSES
    Monitor *monitor = new Monitor(cpu, rom, &clk, &cycles, &address_bus, &data_bus);
#endif

    signal(SIGINT, intHandle);
    cpu->power_on();
    cpu->set_resb_(0);
    while (running)
    {
        clk = (clk + 1) & 1;

        if (clk == 1)
        {
            cycles++;
        }

        if (cycles == 3)
        {
            cpu->set_resb_(1);
        }
        // clock
        rom->set_clk(clk);
        cpu->set_clk(clk);
#ifndef DEBUG_OUT_NO_NCURSES
        monitor->refresh();
        monitor->step();
#else
        std::cout << (int)clk << std::endl;
        std::cin.ignore();
#endif
    }

#ifndef DEBUG_OUT_NO_NCURSES
    monitor->end();
    delete monitor;
#endif
    delete cpu;
    delete rom;
}

void intHandle(int sig)
{
    running = 0;
}