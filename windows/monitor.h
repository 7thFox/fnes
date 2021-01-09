#ifndef MONITOR_H
#define MONITOR_H

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "stdint.h"
#include "../components/cpu6502.h"
#include "../components/rom.h"
#include <string>

typedef struct monitor_s Monitor;

struct monitor_s
{
    WINDOW *win_clock;
    uint8_t *clk;
    int *cycle_count;

    WINDOW *win_data;
    Cpu6502 *cpu;
    uint16_t *address_bus;
    uint8_t *data_bus;

    WINDOW *win_inst;
    Rom *rom;

    WINDOW *win_status;
};

Monitor *monitor_init(
    Cpu6502 *cpu,
    Rom *rom,
    uint8_t *clk, int *cycle_count,
    uint16_t *address_bus, uint8_t *data_bus);

void monitor_refresh(Monitor *monitor);
void monitor_step(Monitor *monitor);

void monitor_end(Monitor *monitor);

#endif