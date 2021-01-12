#ifndef CPU6502_H
#define CPU6502_H

#include "stdint.h"
#include <stdlib.h>
#include <functional>
#include <iostream>

namespace components
{
    class Cpu6502;
    struct _Cpu6502_state
    {
        _Cpu6502_state(std::function<_Cpu6502_state *()> nextstate)
        {
            this->nextstate = nextstate;
        }
        std::function<_Cpu6502_state *()> nextstate;
        _Cpu6502_state *operator()()
        {
            auto next = nextstate();
            return next;
        }
    };

    enum AddressingMode
    {
        A,    // Accumulator
        abs,  // absolute
        absX, // absolute, X-indexed
        absY, // absolute, Y-indexed
        imm,  // immediate
        impl, // implied
        ind,  // indirect
        Xind, // X-indexed, indirect
        indY, // indirect, Y-indexed
        rel,  // relative
        zpg,  // zeropage
        zpgX, // zeropage, X-indexed
        zpgY, // zeropage, Y-indexed
    };

    struct InstructionMetadata
    {
        InstructionMetadata(){
            this->mnemonic = "___";
            this->addr_mode = AddressingMode::impl;
        }
        
        InstructionMetadata(std::string mnemonic,
                            AddressingMode addr_mode,
                            int(Cpu6502::*fnop)())
        {
            this->mnemonic = mnemonic;
            this->addr_mode = addr_mode;
            this->fnop = fnop;
        }
        std::string mnemonic;
        AddressingMode addr_mode;
        uint8_t cycles;
        int (Cpu6502::*fnop)();

        uint8_t nbytes() {
            switch (this->addr_mode)
            {
                case AddressingMode::A:
                case AddressingMode::impl:
                    return 1;
                case AddressingMode::zpg:
                case AddressingMode::imm:
                case AddressingMode::Xind:
                case AddressingMode::indY:
                case AddressingMode::rel:
                case AddressingMode::zpgX:
                case AddressingMode::zpgY:
                    return 2;
                case AddressingMode::abs:
                case AddressingMode::absX:
                case AddressingMode::absY:
                case AddressingMode::ind:
                    return 3;
            }
            return -1;
        }

        bool has_fetch() 
        {
            if (this->addr_mode == AddressingMode::imm ||
                this->addr_mode == AddressingMode::impl ||
                this->mnemonic == "JMP"){
                return false;
            }
            return true;
        }
    };

    enum Cpu6502Flags
    {
        C = 1 << 0,
        Z = 1 << 1,
        I = 1 << 2,
        D = 1 << 3,
        B = 1 << 4,
        S = 1 << 5,
        V = 1 << 6,
        N = 1 << 7,
    };

    class Cpu6502
    {
    public:
        static InstructionMetadata metadata[];

        Cpu6502(uint16_t *addr, uint8_t *data);
        ~Cpu6502();

        void power_on();
        void set_clk();
        void set_resb_(int sig);

        uint8_t get_a();
        uint8_t get_x();
        uint8_t get_y();
        uint8_t get_p();
        uint8_t get_s();
        uint16_t get_pc();
        uint16_t get_inst_start();
        bool get_is_fetching();

        // For testing only
        void test_setup();
        void test_start();
        bool test_finished();
        void test_set_a(uint8_t);
        void test_set_x(uint8_t);
        void test_set_y(uint8_t);
        void test_set_p(uint8_t);
        void test_set_s(uint8_t);
        void test_set_pc(uint16_t);

    private:
        bool is_testing;

        // _Cpu6502_state state();
        _Cpu6502_state *state;

        // internal registers
        uint8_t a;
        uint8_t x;
        uint8_t y;
        uint8_t p;
        uint8_t s;
        uint16_t pc;

        void flg_set(bool set, Cpu6502Flags flag);

        bool is_fetching;

        uint16_t inst_start;
        InstructionMetadata inst_meta;
        uint8_t inst[8];
        union {
            uint8_t param8;// hi byte
            uint16_t param16;
        };

        uint16_t *address; // 4-19 ->
        uint8_t *data;     // 21-28 <->

        // pinout
        // uint8_t ad1 : 1;   // 1 ->
        // uint8_t ad2 : 1;   // 2 ->
        uint8_t resb_ : 1; // 3 <-
        // uint8_t clk : 1;   // 29 <-
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
        uint8_t vcc : 1; // 39 ->

        // state helpers
        _Cpu6502_state *state_wait_cycles(int ncycles, _Cpu6502_state *continue_with);
        // states
        _Cpu6502_state *state_power_off();
        _Cpu6502_state *state_power_on_dirty();
        _Cpu6502_state *state_reset_hold();

        _Cpu6502_state *state_fetch_op();
        _Cpu6502_state *state_fetch_lo();
        _Cpu6502_state *state_fetch_lo_hi();
        _Cpu6502_state *state_fetch_hi();
        _Cpu6502_state *check_indirect();
        _Cpu6502_state *state_fetch_lo_hi_indirect(uint16_t next_address);
        _Cpu6502_state *state_fetch_hi_indirect();
        _Cpu6502_state *check_fetch();
        _Cpu6502_state *state_fetch_addr();

        _Cpu6502_state *state_op_exec();
        _Cpu6502_state *state_prep_fetch();

        _Cpu6502_state *state_hlt();

        // ops
        int op____();
        int op_adc();
        int op_and();
        int op_asl();
        int op_bcc();
        int op_bcs();
        int op_beq();
        int op_bit();
        int op_bmi();
        int op_bne();
        int op_bpl();
        int op_brk();
        int op_bvc();
        int op_bvs();
        int op_clc();
        int op_cld();
        int op_cli();
        int op_clv();
        int op_cmp();
        int op_cpx();
        int op_cpy();
        int op_dec();
        int op_dex();
        int op_dey();
        int op_eor();
        int op_inc();
        int op_inx();
        int op_iny();
        int op_jmp();
        int op_jsr();
        int op_lda();
        int op_ldx();
        int op_ldy();
        int op_lsr();
        int op_nop();
        int op_ora();
        int op_pha();
        int op_php();
        int op_pla();
        int op_plp();
        int op_rol();
        int op_ror();
        int op_rti();
        int op_rts();
        int op_sbc();
        int op_sec();
        int op_sed();
        int op_sei();
        int op_sta();
        int op_stx();
        int op_sty();
        int op_tax();
        int op_tay();
        int op_tsx();
        int op_txa();
        int op_txs();
        int op_tya();    
    };
}
#endif