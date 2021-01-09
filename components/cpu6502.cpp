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

#define m(m, a, c) &InstructionMetadata(m, AddressingMode::a, c)

InstructionMetadata *Cpu6502::metadata[0x100] = {
    m("BRK", impl, -1), m("ORA", Xind, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("ORA", zpg, -1),  m("ASL", zpg, -1),  m("___", impl, -1), m("PHP", impl, -1), m("ORA", imm, -1),  m("ASL", A, -1),    m("___", impl, -1), m("___", impl, -1), m("ORA", abs, -1),  m("ASL", abs, -1),  m("___", impl, -1),
    m("BPL", rel, -1),  m("ORA", indY, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("ORA", zpgX, -1), m("ASL", zpgX, -1), m("___", impl, -1), m("CLC", impl, -1), m("ORA", absY, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("ORA", absX, -1), m("ASL", absX, -1), m("___", impl, -1),
    m("JSR", abs, -1),  m("AND", Xind, -1), m("___", impl, -1), m("___", impl, -1), m("BIT", zpg, -1),  m("AND", zpg, -1),  m("ROL", zpg, -1),  m("___", impl, -1), m("PLP", impl, -1), m("AND", imm, -1),  m("ROL", A, -1),    m("___", impl, -1), m("BIT", abs, -1),  m("AND", abs, -1),  m("ROL", abs, -1),  m("___", impl, -1),
    m("BMI", rel, -1),  m("AND", indY, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("AND", zpgX, -1), m("ROL", zpgX, -1), m("___", impl, -1), m("SEC", impl, -1), m("AND", absY, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("AND", absX, -1), m("ROL", absX, -1), m("___", impl, -1),
    m("RTI", impl, -1), m("EOR", Xind, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("EOR", zpg, -1),  m("LSR", zpg, -1),  m("___", impl, -1), m("PHA", impl, -1), m("EOR", imm, -1),  m("LSR", A, -1),    m("___", impl, -1), m("JMP", abs, -1),  m("EOR", abs, -1),  m("LSR", abs, -1),  m("___", impl, -1),
    m("BVC", rel, -1),  m("EOR", indY, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("EOR", zpgX, -1), m("LSR", zpgX, -1), m("___", impl, -1), m("CLI", impl, -1), m("EOR", absY, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("EOR", absX, -1), m("LSR", absX, -1), m("___", impl, -1),
    m("RTS", impl, -1), m("ADC", Xind, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("ADC", zpg, -1),  m("ROR", zpg, -1),  m("___", impl, -1), m("PLA", impl, -1), m("ADC", imm, -1),  m("ROR", A, -1),    m("___", impl, -1), m("JMP", ind, -1),  m("ADC", abs, -1),  m("ROR", abs, -1),  m("___", impl, -1),
    m("BVS", rel, -1),  m("ADC", indY, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("ADC", zpgX, -1), m("ROR", zpgX, -1), m("___", impl, -1), m("SEI", impl, -1), m("ADC", absY, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("ADC", absX, -1), m("ROR", absX, -1), m("___", impl, -1),
    m("___", impl, -1), m("STA", Xind, -1), m("___", impl, -1), m("___", impl, -1), m("STY", zpg, -1),  m("STA", zpg, -1),  m("STX", zpg, -1),  m("___", impl, -1), m("DEY", impl, -1), m("___", impl, -1), m("TXA", impl, -1), m("___", impl, -1), m("STY", abs, -1),  m("STA", abs, -1),  m("STX", abs, -1),  m("___", impl, -1),
    m("BCC", rel, -1),  m("STA", indY, -1), m("___", impl, -1), m("___", impl, -1), m("STY", zpgX, -1), m("STA", zpgX, -1), m("STX", zpgY, -1), m("___", impl, -1), m("TYA", impl, -1), m("STA", absY, -1), m("TXS", impl, -1), m("___", impl, -1), m("___", impl, -1), m("STA", absX, -1), m("___", impl, -1), m("___", impl, -1),
    m("LDY", imm, -1),  m("LDA", Xind, -1), m("LDX", imm, -1),  m("___", impl, -1), m("LDY", zpg, -1),  m("LDA", zpg, -1),  m("LDX", zpg, -1),  m("___", impl, -1), m("TAY", impl, -1), m("LDA", imm, -1),  m("TAX", impl, -1), m("___", impl, -1), m("LDY", abs, -1),  m("LDA", abs, -1),  m("LDX", abs, -1),  m("___", impl, -1),
    m("BCS", rel, -1),  m("LDA", indY, -1), m("___", impl, -1), m("___", impl, -1), m("LDY", zpgX, -1), m("LDA", zpgX, -1), m("LDX", zpgY, -1), m("___", impl, -1), m("CLV", impl, -1), m("LDA", absY, -1), m("TSX", impl, -1), m("___", impl, -1), m("LDY", absX, -1), m("LDA", absX, -1), m("LDX", absY, -1), m("___", impl, -1),
    m("CPY", imm, -1),  m("CMP", Xind, -1), m("___", impl, -1), m("___", impl, -1), m("CPY", zpg, -1),  m("CMP", zpg, -1),  m("DEC", zpg, -1),  m("___", impl, -1), m("INY", impl, -1), m("CMP", imm, -1),  m("DEX", impl, -1), m("___", impl, -1), m("CPY", abs, -1),  m("CMP", abs, -1),  m("DEC", abs, -1),  m("___", impl, -1),
    m("BNE", rel, -1),  m("CMP", indY, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("CMP", zpgX, -1), m("DEC", zpgX, -1), m("___", impl, -1), m("CLD", impl, -1), m("CMP", absY, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("CMP", absX, -1), m("DEC", absX, -1), m("___", impl, -1),
    m("CPX", imm, -1),  m("SBC", Xind, -1), m("___", impl, -1), m("___", impl, -1), m("CPX", zpg, -1),  m("SBC", zpg, -1),  m("INC", zpg, -1),  m("___", impl, -1), m("INX", impl, -1), m("SBC", imm, -1),  m("NOP", impl, -1), m("___", impl, -1), m("CPX", abs, -1),  m("SBC", abs, -1),  m("INC", abs, -1),  m("___", impl, -1),
    m("BEQ", rel, -1),  m("SBC", indY, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("SBC", zpgX, -1), m("INC", zpgX, -1), m("___", impl, -1), m("SED", impl, -1), m("SBC", absY, -1), m("___", impl, -1), m("___", impl, -1), m("___", impl, -1), m("SBC", absX, -1), m("INC", absX, -1), m("___", impl, -1),
};