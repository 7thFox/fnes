#ifndef CPU6502_H
#define CPU6502_H

#include "stdint.h"
#include <stdlib.h>

typedef struct cpu6502_s Cpu6502;

struct cpu6502_s
{
    // internal registers
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t p;
    uint8_t s;
    uint16_t pc;

    uint8_t inst[8];

    uint16_t *address; // 4-19 ->
    uint8_t *data;     // 21-28 <->

    uint8_t emu_cycles;

    // pinout
    // uint8_t ad1 : 1;   // 1 ->
    // uint8_t ad2 : 1;   // 2 ->
    uint8_t resb_ : 1; // 3 <-
    uint8_t clk : 1;   // 29 <-
    // uint8_t tst : 1;   // 30 <-
    // uint8_t m2 : 1;    // 31 ->
    // uint8_t irqb_ : 1; // 32 <-
    // uint8_t nmib_ : 1; // 33 <-
    // uint8_t rwb : 1;   // 34 ->
    // uint8_t oe2_ : 1;  // 35 ->
    // uint8_t oe1_ : 1;  // 36 ->
    // uint8_t out2 : 1;  // 37 ->
    // uint8_t out1 : 1;  // 38 ->
    // uint8_t out0 : 1;  // 39 ->
};

// setup
Cpu6502 *cpu_new(uint16_t *addr, uint8_t *data);
void cpu_power_on(Cpu6502 *cpu);
// void cpu_addr_connect(Cpu6502 *cpu, uint16_t *addr);
// void cpu_data_connect(Cpu6502 *cpu, uint8_t *data);

// uint16_t *cpu_addr(Cpu6502 *cpu);
// uint8_t *cpu_data(Cpu6502 *cpu);

// inputs
void cpu_clk(Cpu6502 *cpu, int sig);
void cpu_resb_(Cpu6502 *cpu, int sig);

#endif