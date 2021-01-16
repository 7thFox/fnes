#include "signal.h"
#include <cstdlib>
#include <stdlib.h>
#include "unistd.h"
#include <iostream>
#include "tests_instructions.h"
#include "components/rom.h"
#include "components/cpu6502.h"
#include "windows/monitor.h"

int running = 1;
void intHandle(int);

//#define DEBUG_OUT_NO_NCURSES
void run_interactive();

int main(int argc, char *argv[])
{

    // TODO JOSH: You don't execute FFFC/FFFD, they're the PC of where you start lol
    bool performTest = false;
    bool verboseTest = false;
    std::srand(std::time(nullptr));
    for (int i = 0; i < argc; i++)
    {
        auto arg = std::string(argv[i]);
        if (arg == "-t" || arg == "--test")
        {
            performTest = true;
        }
        else if (arg == "-v" || arg == "--verbose")
        {
            verboseTest = true;
        }
    }

    if (performTest)
    {
        test::run_all_instruction_tests(&std::cout, !verboseTest);
    }
    else
    {
        run_interactive();
    }
}

void run_interactive()
{
    uint8_t prg[100] = {
        0xA9, 0x14,       // LDA #$14
        0x69, 0x43,       // ADC #$43 // A = $57

        0xA2, 0x00, // LDX #$00
        0xAD, 0x50, 0x50, // LDA $5050 (33)
        0xBD, 0x50, 0x50, // LDA $5050,X
        0xE8,// INX
        0xBD, 0x50, 0x50, // LDA $5050,X
        0xE8,// INX
        0xBD, 0x50, 0x50, // LDA $5050,X
        0xE8,// INX
        0xBD, 0x50, 0x50, // LDA $5050,X
        0xE8,// INX

        0x4C, 0x20, 0x40, // JMP $4020
    };
    uint8_t image[0xBFE0];
    memcpy(image, prg, 100);
    image[0xFFFC - 0x4020] = 0x4C; // JMP $4020 (start of ROM)
    image[0xFFFD - 0x4020] = 0x20;
    image[0xFFFE - 0x4020] = 0x40;


    image[0x5050 - 0x4020] = 0x02;
    image[0x5051 - 0x4020] = 0x03;
    image[0x5052 - 0x4020] = 0x07;
    image[0x5053 - 0x4020] = 0x13;
    image[0x5054 - 0x4020] = 0x17;
    image[0x5055 - 0x4020] = 0x23;

    uint8_t data_bus;
    uint16_t address_bus;
    components::Cpu6502 *cpu = new components::Cpu6502(&address_bus, &data_bus);
    components::Rom *rom = new components::Rom(image, &address_bus, &data_bus);
    components::Ram *ram = new components::Ram(&address_bus, &data_bus);

    // uint8_t clk = 0;
    int cycles = 0;
#ifndef DEBUG_OUT_NO_NCURSES
    Monitor *monitor = new Monitor(cpu, rom, ram, &cycles, &address_bus, &data_bus);
#endif

    signal(SIGINT, intHandle);
    cpu->power_on();
    cpu->set_resb_(0);
    while (running)
    {
        // clk = (clk + 1) & 1;

        // if (clk == 1)
        // {
        cycles++;
        // }

        if (cycles == 3)
        {
            cpu->set_resb_(1);
        }
        // clock
        rom->set_clk();
        cpu->set_clk();
#ifndef DEBUG_OUT_NO_NCURSES
        monitor->refresh();
        monitor->step();
#else
        // std::cout << (int)clk << std::endl;
        std::cin.ignore();
#endif
    }

#ifndef DEBUG_OUT_NO_NCURSES
    monitor->end();
    delete monitor;
#endif
    delete cpu;
    delete rom;
    delete ram;
}

void intHandle(int sig)
{
    running = 0;
}