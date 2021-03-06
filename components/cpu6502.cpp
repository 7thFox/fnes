#include "cpu6502.h"

namespace components
{
Cpu6502::Cpu6502(uint16_t *addr, uint8_t *data) {
    this->address = addr;
    this->data = data;
    this->vcc = 0;
    this->is_fetching = false;
    this->is_testing = false;
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
void Cpu6502::flg_set(bool set, Cpu6502Flags flag)
{
    if (set)
        this->p |= flag;
    else
        this->p &= ~flag;
}

uint8_t Cpu6502::get_a() { return this->a; }
uint8_t Cpu6502::get_x() { return this->x; }
uint8_t Cpu6502::get_y() { return this->y; }
uint8_t Cpu6502::get_p() { return this->p; }
uint8_t Cpu6502::get_s() { return this->s; }
uint16_t Cpu6502::get_pc() { return this->pc; }
uint16_t Cpu6502::get_inst_start() { return this->inst_start; }
bool Cpu6502::get_is_fetching() { return this->is_fetching; }


void Cpu6502::test_setup()
{
    this->is_testing = true;
    this->pc = 0xFFFC;
}
void Cpu6502::test_start()
{
    if (this->state != nullptr){ delete this->state; }
    this->state = this->state_prep_fetch();
    this->is_testing = true;// set false by prep_fetch
}
bool Cpu6502::test_finished() 
{ 
    return !this->is_testing; 
}
void Cpu6502::test_set_a(uint8_t x) { if (this->is_testing) this->a = x; }
void Cpu6502::test_set_x(uint8_t x) { if (this->is_testing) this->x = x; }
void Cpu6502::test_set_y(uint8_t x) { if (this->is_testing) this->y = x; }
void Cpu6502::test_set_p(uint8_t x) { if (this->is_testing) this->p = x; }
void Cpu6502::test_set_s(uint8_t x) { if (this->is_testing) this->s = x; }
void Cpu6502::test_set_pc(uint16_t x) { if (this->is_testing) this->pc = x; }
uint16_t Cpu6502::test_get_addr() { return *this->address; }
uint8_t Cpu6502::test_get_data() { return *this->data; }

_Cpu6502_state *Cpu6502::state_wait_cycles(int ncycles, _Cpu6502_state *continue_with)
{
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
    if (this->vcc)
    {
        return new _Cpu6502_state(std::bind(&Cpu6502::state_power_on_dirty, this));
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::state_power_off, this));
}

_Cpu6502_state *Cpu6502::state_power_on_dirty()
{
    if (!this->resb_)
    {
        return new _Cpu6502_state(std::bind(&Cpu6502::state_reset_hold, this));
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::state_power_on_dirty, this));
}

_Cpu6502_state *Cpu6502::state_reset_hold()
{
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
    this->is_fetching = false;
    this->inst_meta = Cpu6502::metadata[this->inst[0]];
    auto nbytes = inst_meta.nbytes();
    if (nbytes == 1)
    {
        return this->check_indirect();
    }

    *this->address = this->pc;
    this->pc++;
    if (nbytes == 2){
        return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_lo, this));
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_lo_hi, this));
}

_Cpu6502_state *Cpu6502::state_fetch_lo(){
    this->inst[1] = *this->data;
    return this->check_indirect();
}

_Cpu6502_state *Cpu6502::state_fetch_lo_hi(){
    this->inst[1] = *this->data;
    *this->address = this->pc;
    this->pc++;
    return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_hi, this));
}

_Cpu6502_state *Cpu6502::state_fetch_hi(){
    this->inst[2] = *this->data;
    return this->check_indirect();
}

_Cpu6502_state *Cpu6502::state_fetch_lo_hi_indirect(uint16_t next_address){
    this->inst[1] = *this->data;
    *this->address = next_address;
    return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_hi_indirect, this));
}

_Cpu6502_state *Cpu6502::state_fetch_hi_indirect() {
    this->inst[2] = *this->data;
    this->param16 = this->inst[1] | (this->inst[2] << 8);
    if (this->inst_meta.addr_mode == AddressingMode::indY)
    {
        this->param16 += this->y;
        if (this->inst_meta.always_page() ||this->param16 & 0xFF00 != this->inst[2] << 8)
        {
            return new _Cpu6502_state(std::bind(&Cpu6502::check_fetch, this));
        }
        return this->check_fetch();
    }
    return new _Cpu6502_state(std::bind(&Cpu6502::check_fetch, this));
}
_Cpu6502_state *Cpu6502::check_indirect(){
    switch (this->inst_meta.addr_mode)
    {
        case AddressingMode::A:
            this->param8 = this->a;
            break;
        case AddressingMode::imm:
            this->param8 = this->inst[1];
            break;
        case AddressingMode::impl:
            break;            
        case AddressingMode::zpg:
            this->param16 = this->inst[1];
            break;
        case AddressingMode::zpgX:
            this->param16 = (this->inst[1] + this->x) & 0xFF;
            return new _Cpu6502_state(std::bind(&Cpu6502::check_fetch, this));// Adding a cycle for LDA zpg,X. We'll see if it carries
            // break;
        case AddressingMode::zpgY:
            this->param16 = (this->inst[1] + this->y) & 0xFF;
            break;
        case AddressingMode::abs:
            this->param16 = this->inst[1] | (this->inst[2] << 8);
            break;
        case AddressingMode::absX:
            this->param16 = (this->inst[1] | (this->inst[2] << 8)) + x;
            if (this->inst_meta.always_page() || this->param16 & 0xFF00 != this->inst[2] << 8)
            {
                return new _Cpu6502_state(std::bind(&Cpu6502::check_fetch, this));
            }
            break;
        case AddressingMode::absY:
            this->param16 = (this->inst[1] | (this->inst[2] << 8)) + y;
            if (this->inst_meta.always_page() ||this->param16 & 0xFF00 != this->inst[2] << 8)
            {
                return new _Cpu6502_state(std::bind(&Cpu6502::check_fetch, this));
            }
            break;
        case AddressingMode::ind:
            *this->address = this->inst[1] | (this->inst[2] << 8);
            return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_lo_hi_indirect, this,
                (this->inst[1] + 1) & 0xFF | this->inst[2] << 8));
        case AddressingMode::Xind:
            *this->address = (this->inst[1] + this->x) & 0xFF;
            return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_lo_hi_indirect, this,
                (this->inst[1] + this->x + 1) & 0xFF));
        case AddressingMode::indY:
            *this->address = this->inst[1];
            return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_lo_hi_indirect, this,
                (this->inst[1] + 1) & 0xFF));
        case AddressingMode::rel:
            this->param8 = this->inst[1];
            if ((this->pc + this->param8_signed) & 0xFF00 != this->pc & 0xFF00)
            {
                return new _Cpu6502_state(std::bind(&Cpu6502::check_fetch, this));
            }
            // if (this->param16 & 0x08)
            //     this->param16 |= 0xFF00;
            // this->param16 += this->pc;
            break;
    }
    return this->check_fetch();
}

_Cpu6502_state *Cpu6502::check_fetch(){
    if (!this->inst_meta.has_fetch())
    {
        return this->state_op_exec();
    }

    *this->address = this->param16;
    return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_addr, this));
}
_Cpu6502_state *Cpu6502::state_fetch_addr()
{
    this->param8 = *this->data;
    return this->state_op_exec();
}

_Cpu6502_state *Cpu6502::state_op_exec(){
    auto cycles = std::bind(this->inst_meta.fnop, this)();
    if (cycles > 0)
    {
        auto fetch_next_op = new _Cpu6502_state(std::bind(&Cpu6502::state_prep_fetch, this));
        if (cycles > 1){
            return new _Cpu6502_state(std::bind(&Cpu6502::state_wait_cycles, this, cycles - 1, fetch_next_op));
        }
        return fetch_next_op;
    }
    return this->state_prep_fetch();
}

_Cpu6502_state *Cpu6502::state_prep_fetch()
{
    this->is_testing = false;
    this->inst_start = this->pc;
    *this->address = this->pc;
    this->is_fetching = true;
    return new _Cpu6502_state(std::bind(&Cpu6502::state_fetch_op, this));
}

_Cpu6502_state *Cpu6502::state_hlt(){
    return new _Cpu6502_state(std::bind(&Cpu6502::state_hlt, this));
}

int Cpu6502::op____() { return 0; }
int Cpu6502::op_adc() 
{
    auto tmp = (uint16_t)this->a + (uint16_t)this->param8;
    this->flg_set((tmp & 0x80) == 0x80, Cpu6502Flags::N);
    this->flg_set((tmp & 0xFF) == 0, Cpu6502Flags::Z);
    this->flg_set(tmp > 0xFF, Cpu6502Flags::C);
    this->flg_set((~((uint16_t)this->a ^ (uint16_t)this->param8) & ((uint16_t)this->a ^ tmp)) & 0x0080, Cpu6502Flags::V);
    this->a = tmp;
    return 0; 
}
int Cpu6502::op_and()
{ 
    this->a &= this->param8;
    this->flg_set(this->a == 0, Cpu6502Flags::Z);
    this->flg_set((this->a & 0x80) == 0x80, Cpu6502Flags::N);
    return 0;
}
int Cpu6502::op_asl()
{ 
    this->flg_set((this->param8 & 0x80) == 0x80, Cpu6502Flags::C);
    this->a = (this->param8 << 1) & 0xFF;
    this->flg_set(this->a == 0, Cpu6502Flags::Z);
    this->flg_set((this->a & 0x80) == 0x80, Cpu6502Flags::N);
    return this->inst_meta.addr_mode == AddressingMode::A ? 1 : 2;
}
int Cpu6502::op_bcc()
{
    if ((this->p & Cpu6502Flags::C) != Cpu6502Flags::C)
    {
        this->pc = (uint16_t)((this->pc + this->param8_signed) - 1);
    }
    return 1;
}
int Cpu6502::op_bcs() 
{
    if ((this->p & Cpu6502Flags::C) == Cpu6502Flags::C)
    {
        this->pc = (uint16_t)((this->pc + this->param8_signed) - 1);
    }
    return 1;
}
int Cpu6502::op_beq()
{
    if ((this->p & Cpu6502Flags::Z) == Cpu6502Flags::Z)
    {
        this->pc = (uint16_t)((this->pc + this->param8_signed) - 1);
    }
    return 1;
}
int Cpu6502::op_bit()
{
    this->flg_set((this->param8 & (1 << 7)) == (1 << 7), Cpu6502Flags::N);
    this->flg_set((this->param8 & (1 << 6)) == (1 << 6), Cpu6502Flags::V);
    this->flg_set((this->param8 & this->a) == 0, Cpu6502Flags::Z);
    return 0;
}
int Cpu6502::op_bmi()
{
    if ((this->p & Cpu6502Flags::N) == Cpu6502Flags::N)
    {
        this->pc = (uint16_t)((this->pc + this->param8_signed) - 1);
    }
    return 1;
}
int Cpu6502::op_bne()
{
    if ((this->p & Cpu6502Flags::Z) != Cpu6502Flags::Z)
    {
        this->pc = (uint16_t)((this->pc + this->param8_signed) - 1);
    }
    return 1;
}
int Cpu6502::op_bpl()
{
    if ((this->p & Cpu6502Flags::N) != Cpu6502Flags::N)
    {
        this->pc = (uint16_t)((this->pc + this->param8_signed) - 1);
    }
    return 1;
}
int Cpu6502::op_brk() { }
int Cpu6502::op_bvc()
{
    if ((this->p & Cpu6502Flags::V) != Cpu6502Flags::V)
    {
        this->pc = (uint16_t)((this->pc + this->param8_signed) - 1);
    }
    return 1;
}
int Cpu6502::op_bvs()
{
    if ((this->p & Cpu6502Flags::V) == Cpu6502Flags::V)
    {
        this->pc = (uint16_t)((this->pc + this->param8_signed) - 1);
    }
    return 1;
}
// TODO: Remove when implemented
#pragma GCC diagnostic ignored "-Wreturn-type"
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
int Cpu6502::op_inx() { this->x++; return 0; }
int Cpu6502::op_iny() { }
int Cpu6502::op_jmp() { this->pc = this->param16; return 0; }
int Cpu6502::op_jsr() { }
int Cpu6502::op_lda() 
{ 
    this->a = this->param8;
    this->flg_set(this->a == 0, Cpu6502Flags::Z);
    this->flg_set((this->a & 0x80) == 0x80, Cpu6502Flags::N);
    return 0;
}
int Cpu6502::op_ldx() { this->x = this->param8; return 0; }
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
 
#pragma GCC diagnostic push




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