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

class Monitor
{
public:
    Monitor(
        components::Cpu6502 *cpu,
        components::Rom *rom,
        int *cycle_count,
        uint16_t *address_bus, uint8_t *data_bus);
    ~Monitor();

    void refresh();
    void step();

    void end();

private:
    WINDOW *win_clock;
    // uint8_t *clk;
    int *cycle_count;

    static char FLAG_NAMES[];
    WINDOW *win_data;
    components::Cpu6502 *cpu;
    uint16_t *address_bus;
    uint8_t *data_bus;

    WINDOW *win_inst;
    components::Rom *rom;

    WINDOW *win_status;

    typedef int _box_intersects;
    const int UP = 1 << 0;
    const int LEFT = 1 << 1;
    const int RIGHT = 1 << 2;
    const int DOWN = 1 << 3;

    void draw_data();
    void draw_clock();
    void draw_inst();
    int enter_command_mode();
    void set_status(std::string str);
    void box_draw(WINDOW *win, _box_intersects intersect, chtype ul, chtype ur, chtype ll, chtype lr);
};

#endif