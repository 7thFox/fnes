#include "cpu6502.h"

namespace components
{
Cpu6502::Cpu6502(uint16_t *addr, uint8_t *data) {
    this->address = addr;
    this->data = data;
    this->vcc = 0;
    this->is_fetching = false;
    this->state = new _Cpu6502_state(std::bind(&Cpu6502::state_power_off, this));
}

Cpu6502::~Cpu6502()
{
    delete this->state;
}

void Cpu6502::set_clk()
{
    auto next = (*this->state)();
    delete this->state;
    this->state = next;
}

void Cpu6502::power_on()
{
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
        this->inst_start = this->pc = 0xFFFC;
        
        //cpu->irq = true;
    }
}

uint8_t Cpu6502::get_a() { return this->a; }
uint8_t Cpu6502::get_x() { return this->x; }
uint8_t Cpu6502::get_y() { return this->y; }
uint8_t Cpu6502::get_p() { return this->p; }
uint8_t Cpu6502::get_s() { return this->s; }
uint16_t Cpu6502::get_pc() { return this->pc; }
uint16_t Cpu6502::get_inst_start() { return this->inst_start; }
bool Cpu6502::get_is_fetching() { return this->is_fetching; }

_Cpu6502_state *Cpu6502::state_wait_cycles(int ncycles, _Cpu6502_state *continue_with)
{
    std::cout << "state_wait_cycles " << ncycles << std::endl;
    if (ncycles == 1)
    {
        return continue_with;
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::state_wait_cycles, this, ncycles - 1, continue_with));
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
        auto cnt = new _Cpu6502_state(std::bind(&Cpu6502::state_prep_fetch, this));
        return new _Cpu6502_state(std::bind(&Cpu6502::state_wait_cycles, this, 1, cnt));
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::state_reset_hold, this));
}
_Cpu6502_state *Cpu6502::state_fetch_op(){
    this->inst[0] = *this->data;
    this->pc++;
    this->inst_meta = Cpu6502::metadata[this->inst[0]];
    auto nbytes = inst_meta.nbytes();
    if (nbytes < 2){
        this->is_fetching = false;
        return new _Cpu6502_state(std::bind(&Cpu6502::state_op_exec, this));
    }

    *this->address = this->pc;
    if (nbytes == 2){
        return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_lo, this));
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_lo_hi, this));
}
_Cpu6502_state *Cpu6502::state_fetch_lo(){
    this->inst[1] = *this->data;
    this->pc++;
    this->is_fetching = false;
    return new _Cpu6502_state(std::bind(&Cpu6502::state_op_exec, this));
}
_Cpu6502_state *Cpu6502::state_fetch_lo_hi(){
    this->inst[1] = *this->data;
    this->pc++;
    *this->address = this->pc;
    return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_hi, this));
}
_Cpu6502_state *Cpu6502::state_fetch_hi(){
    this->inst[2] = *this->data;
    this->pc++;
    this->is_fetching = false;
    return new _Cpu6502_state(std::bind(&Cpu6502::state_op_exec, this));
}

_Cpu6502_state *Cpu6502::state_op_exec(){
    auto cycles = std::bind(this->inst_meta.fnop, this)();
    if (cycles > 0)
    {
        auto fetch = new _Cpu6502_state(std::bind(&Cpu6502::state_prep_fetch, this));
        if (cycles > 1){
            return new _Cpu6502_state(std::bind(&Cpu6502::state_wait_cycles, this, cycles - 1, fetch));
        }
        return fetch;
    }
    return this->state_prep_fetch();
}

_Cpu6502_state *Cpu6502::state_prep_fetch()
{
    this->inst_start = this->pc;
    *this->address = this->pc;
    this->is_fetching = true;
    return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_op, this));
}

_Cpu6502_state *Cpu6502::state_hlt(){
    return new _Cpu6502_state(std::bind(&Cpu6502::state_hlt, this));
}

int Cpu6502::op____() { }
int Cpu6502::op_adc() { }
int Cpu6502::op_and() { }
int Cpu6502::op_asl() { }
int Cpu6502::op_bcc() { }
int Cpu6502::op_bcs() { }
int Cpu6502::op_beq() { }
int Cpu6502::op_bit() { }
int Cpu6502::op_bmi() { }
int Cpu6502::op_bne() { }
int Cpu6502::op_bpl() { }
int Cpu6502::op_brk() { }
int Cpu6502::op_bvc() { }
int Cpu6502::op_bvs() { }
int Cpu6502::op_clc() { }
int Cpu6502::op_cld() { }
int Cpu6502::op_cli() { }
int Cpu6502::op_clv() { }
int Cpu6502::op_cmp() { }
int Cpu6502::op_cpx() { }
int Cpu6502::op_cpy() { }
int Cpu6502::op_dec() { }
int Cpu6502::op_dex() { }
int Cpu6502::op_dey() { }
int Cpu6502::op_eor() { }
int Cpu6502::op_inc() { }
int Cpu6502::op_inx() { }
int Cpu6502::op_iny() { }
int Cpu6502::op_jmp() 
{
    this->pc = this->inst[1] | (this->inst[2] << 8);
    return 0;
}
int Cpu6502::op_jsr() { }
int Cpu6502::op_lda() { }
int Cpu6502::op_ldx() { }
int Cpu6502::op_ldy() { }
int Cpu6502::op_lsr() { }
int Cpu6502::op_nop() { }
int Cpu6502::op_ora() { }
int Cpu6502::op_pha() { }
int Cpu6502::op_php() { }
int Cpu6502::op_pla() { }
int Cpu6502::op_plp() { }
int Cpu6502::op_rol() { }
int Cpu6502::op_ror() { }
int Cpu6502::op_rti() { }
int Cpu6502::op_rts() { }
int Cpu6502::op_sbc() { }
int Cpu6502::op_sec() { }
int Cpu6502::op_sed() { }
int Cpu6502::op_sei() { }
int Cpu6502::op_sta() { }
int Cpu6502::op_stx() { }
int Cpu6502::op_sty() { }
int Cpu6502::op_tax() { }
int Cpu6502::op_tay() { }
int Cpu6502::op_tsx() { }
int Cpu6502::op_txa() { }
int Cpu6502::op_txs() { }
int Cpu6502::op_tya() { }
 




// #define c Cpu6502
#define m(m, a, f) InstructionMetadata(m, AddressingMode::a, &Cpu6502::f)

InstructionMetadata Cpu6502::metadata[0x100] = {
    
    m("BRK", impl, op_brk), m("ORA", Xind, op_ora), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("ORA", zpg, op_ora),  m("ASL", zpg, op_asl),  m("___", impl, op____), m("PHP", impl, op_php), m("ORA", imm, op_ora),  m("ASL", A, op_asl),    m("___", impl, op____), m("___", impl, op____), m("ORA", abs, op_ora),  m("ASL", abs, op_asl),  m("___", impl, op____),
    m("BPL", rel, op_bpl),  m("ORA", indY, op_ora), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("ORA", zpgX, op_ora), m("ASL", zpgX, op_asl), m("___", impl, op____), m("CLC", impl, op_clc), m("ORA", absY, op_ora), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("ORA", absX, op_ora), m("ASL", absX, op_asl), m("___", impl, op____),
    m("JSR", abs, op_jsr),  m("AND", Xind, op_and), m("___", impl, op____), m("___", impl, op____), m("BIT", zpg, op_bit),  m("AND", zpg, op_and),  m("ROL", zpg, op_rol),  m("___", impl, op____), m("PLP", impl, op_plp), m("AND", imm, op_and),  m("ROL", A, op_rol),    m("___", impl, op____), m("BIT", abs, op_bit),  m("AND", abs, op_and),  m("ROL", abs, op_rol),  m("___", impl, op____),
    m("BMI", rel, op_bmi),  m("AND", indY, op_and), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("AND", zpgX, op_and), m("ROL", zpgX, op_rol), m("___", impl, op____), m("SEC", impl, op_sec), m("AND", absY, op_and), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("AND", absX, op_and), m("ROL", absX, op_rol), m("___", impl, op____),
    m("RTI", impl, op_rti), m("EOR", Xind, op_eor), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("EOR", zpg, op_eor),  m("LSR", zpg, op_lsr),  m("___", impl, op____), m("PHA", impl, op_pha), m("EOR", imm, op_eor),  m("LSR", A, op_lsr),    m("___", impl, op____), m("JMP", abs, op_jmp),  m("EOR", abs, op_eor),  m("LSR", abs, op_lsr),  m("___", impl, op____),
    m("BVC", rel, op_bvc),  m("EOR", indY, op_eor), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("EOR", zpgX, op_eor), m("LSR", zpgX, op_lsr), m("___", impl, op____), m("CLI", impl, op_cli), m("EOR", absY, op_eor), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("EOR", absX, op_eor), m("LSR", absX, op_lsr), m("___", impl, op____),
    m("RTS", impl, op_rts), m("ADC", Xind, op_adc), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("ADC", zpg, op_adc),  m("ROR", zpg, op_ror),  m("___", impl, op____), m("PLA", impl, op_pla), m("ADC", imm, op_adc),  m("ROR", A, op_ror),    m("___", impl, op____), m("JMP", ind, op_jmp),  m("ADC", abs, op_adc),  m("ROR", abs, op_ror),  m("___", impl, op____),
    m("BVS", rel, op_bvs),  m("ADC", indY, op_adc), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("ADC", zpgX, op_adc), m("ROR", zpgX, op_ror), m("___", impl, op____), m("SEI", impl, op_sei), m("ADC", absY, op_adc), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("ADC", absX, op_adc), m("ROR", absX, op_ror), m("___", impl, op____),
    m("___", impl, op____), m("STA", Xind, op_sta), m("___", impl, op____), m("___", impl, op____), m("STY", zpg, op_sty),  m("STA", zpg, op_sta),  m("STX", zpg, op_stx),  m("___", impl, op____), m("DEY", impl, op_dey), m("___", impl, op____), m("TXA", impl, op_txa), m("___", impl, op____), m("STY", abs, op_sty),  m("STA", abs, op_sta),  m("STX", abs, op_stx),  m("___", impl, op____),
    m("BCC", rel, op_bcc),  m("STA", indY, op_sta), m("___", impl, op____), m("___", impl, op____), m("STY", zpgX, op_sty), m("STA", zpgX, op_sta), m("STX", zpgY, op_stx), m("___", impl, op____), m("TYA", impl, op_tya), m("STA", absY, op_sta), m("TXS", impl, op_txs), m("___", impl, op____), m("___", impl, op____), m("STA", absX, op_sta), m("___", impl, op____), m("___", impl, op____),
    m("LDY", imm, op_ldy),  m("LDA", Xind, op_lda), m("LDX", imm, op_ldx),  m("___", impl, op____), m("LDY", zpg, op_ldy),  m("LDA", zpg, op_lda),  m("LDX", zpg, op_ldx),  m("___", impl, op____), m("TAY", impl, op_tay), m("LDA", imm, op_lda),  m("TAX", impl, op_tax), m("___", impl, op____), m("LDY", abs, op_ldy),  m("LDA", abs, op_lda),  m("LDX", abs, op_ldx),  m("___", impl, op____),
    m("BCS", rel, op_bcs),  m("LDA", indY, op_lda), m("___", impl, op____), m("___", impl, op____), m("LDY", zpgX, op_ldy), m("LDA", zpgX, op_lda), m("LDX", zpgY, op_ldx), m("___", impl, op____), m("CLV", impl, op_clv), m("LDA", absY, op_lda), m("TSX", impl, op_tsx), m("___", impl, op____), m("LDY", absX, op_ldy), m("LDA", absX, op_lda), m("LDX", absY, op_ldx), m("___", impl, op____),
    m("CPY", imm, op_cpy),  m("CMP", Xind, op_cmp), m("___", impl, op____), m("___", impl, op____), m("CPY", zpg, op_cpy),  m("CMP", zpg, op_cmp),  m("DEC", zpg, op_dec),  m("___", impl, op____), m("INY", impl, op_iny), m("CMP", imm, op_cmp),  m("DEX", impl, op_dex), m("___", impl, op____), m("CPY", abs, op_cpy),  m("CMP", abs, op_cmp),  m("DEC", abs, op_dec),  m("___", impl, op____),
    m("BNE", rel, op_bne),  m("CMP", indY, op_cmp), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("CMP", zpgX, op_cmp), m("DEC", zpgX, op_dec), m("___", impl, op____), m("CLD", impl, op_cld), m("CMP", absY, op_cmp), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("CMP", absX, op_cmp), m("DEC", absX, op_dec), m("___", impl, op____),
    m("CPX", imm, op_cpx),  m("SBC", Xind, op_sbc), m("___", impl, op____), m("___", impl, op____), m("CPX", zpg, op_cpx),  m("SBC", zpg, op_sbc),  m("INC", zpg, op_inc),  m("___", impl, op____), m("INX", impl, op_inx), m("SBC", imm, op_sbc),  m("NOP", impl, op_nop), m("___", impl, op____), m("CPX", abs, op_cpx),  m("SBC", abs, op_sbc),  m("INC", abs, op_inc),  m("___", impl, op____),
    m("BEQ", rel, op_beq),  m("SBC", indY, op_sbc), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("SBC", zpgX, op_sbc), m("INC", zpgX, op_inc), m("___", impl, op____), m("SED", impl, op_sed), m("SBC", absY, op_sbc), m("___", impl, op____), m("___", impl, op____), m("___", impl, op____), m("SBC", absX, op_sbc), m("INC", absX, op_inc), m("___", impl, op____),

};
}