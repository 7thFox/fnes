#ifndef TESTS_INSTRUCTIONS_H
#define TESTS_INSTRUCTIONS_H

#include "test.h"
#include "components/cpu6502.h"
#include <stdint.h>
#include <functional>
#include <unordered_map>
#include <iostream>

namespace test
{
    void run_all_instruction_tests(std::ostream& out, bool results_only);

    test::TestResult test_inst(
        std::string test_name,
        std::unordered_map<uint16_t, uint8_t> *mem,
        int cycles, std::function<bool(components::Cpu6502 *)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup = {});

    test::TestResult test_range_8(std::function<test::TestResult(uint8_t)> test_fn, uint8_t lo = 0x00, uint8_t hi = 0xFF);
    test::TestResult test_range_16(std::function<test::TestResult(uint16_t)> test_fn, uint16_t lo = 0x0000, uint16_t hi = 0xFFFF);

    // test::TestResult test_ADC_Xind();
    // test::TestResult test_ADC_abs();
    // test::TestResult test_ADC_absX();
    // test::TestResult test_ADC_absY();
    // test::TestResult test_ADC_imm();
    // test::TestResult test_ADC_indY();
    // test::TestResult test_ADC_zpg();
    // test::TestResult test_ADC_zpgX();

    // test::TestResult test_AND_Xind();
    // test::TestResult test_AND_abs();
    // test::TestResult test_AND_absX();
    // test::TestResult test_AND_absY();
    // test::TestResult test_AND_imm();
    // test::TestResult test_AND_indY();
    // test::TestResult test_AND_zpg();
    // test::TestResult test_AND_zpgX();

    // test::TestResult test_ASL_A();
    // test::TestResult test_ASL_abs();
    // test::TestResult test_ASL_absX();
    // test::TestResult test_ASL_zpg();
    // test::TestResult test_ASL_zpgX();

    // test::TestResult test_BCC_rel();

    // test::TestResult test_BCS_rel();

    // test::TestResult test_BEQ_rel();

    // test::TestResult test_BIT_abs();
    // test::TestResult test_BIT_zpg();

    // test::TestResult test_BMI_rel();

    // test::TestResult test_BNE_rel();

    // test::TestResult test_BPL_rel();

    // test::TestResult test_BRK_impl();

    // test::TestResult test_BVC_rel();

    // test::TestResult test_BVS_rel();

    // test::TestResult test_CLC_impl();

    // test::TestResult test_CLD_impl();

    // test::TestResult test_CLI_impl();

    // test::TestResult test_CLV_impl();

    // test::TestResult test_CMP_Xind();
    // test::TestResult test_CMP_abs();
    // test::TestResult test_CMP_absX();
    // test::TestResult test_CMP_absY();
    // test::TestResult test_CMP_imm();
    // test::TestResult test_CMP_indY();
    // test::TestResult test_CMP_zpg();
    // test::TestResult test_CMP_zpgX();

    // test::TestResult test_CPX_abs();
    // test::TestResult test_CPX_imm();
    // test::TestResult test_CPX_zpg();

    // test::TestResult test_CPY_abs();
    // test::TestResult test_CPY_imm();
    // test::TestResult test_CPY_zpg();

    // test::TestResult test_DEC_abs();
    // test::TestResult test_DEC_absX();
    // test::TestResult test_DEC_zpg();
    // test::TestResult test_DEC_zpgX();

    // test::TestResult test_DEX_impl();

    // test::TestResult test_DEY_impl();

    // test::TestResult test_EOR_Xind();
    // test::TestResult test_EOR_abs();
    // test::TestResult test_EOR_absX();
    // test::TestResult test_EOR_absY();
    // test::TestResult test_EOR_imm();
    // test::TestResult test_EOR_indY();
    // test::TestResult test_EOR_zpg();
    // test::TestResult test_EOR_zpgX();

    // test::TestResult test_INC_abs();
    // test::TestResult test_INC_absX();
    // test::TestResult test_INC_zpg();
    // test::TestResult test_INC_zpgX();
    // test::TestResult test_INX_impl();
    // test::TestResult test_INY_impl();

    // test::TestResult test_JMP_abs();
    // test::TestResult test_JMP_ind();

    // test::TestResult test_JSR_abs();

    test::TestResult test_LDA_Xind();
    test::TestResult test_LDA_abs();
    test::TestResult test_LDA_absX();
    test::TestResult test_LDA_absY();
    test::TestResult test_LDA_imm();
    test::TestResult test_LDA_indY();
    test::TestResult test_LDA_zpg();
    test::TestResult test_LDA_zpgX();

    // test::TestResult test_LDX_abs();
    // test::TestResult test_LDX_absY();
    // test::TestResult test_LDX_imm();
    // test::TestResult test_LDX_zpg();
    // test::TestResult test_LDX_zpgY();

    // test::TestResult test_LDY_abs();
    // test::TestResult test_LDY_absX();
    // test::TestResult test_LDY_imm();
    // test::TestResult test_LDY_zpg();
    // test::TestResult test_LDY_zpgX();

    // test::TestResult test_LSR_A();
    // test::TestResult test_LSR_abs();
    // test::TestResult test_LSR_absX();
    // test::TestResult test_LSR_zpg();
    // test::TestResult test_LSR_zpgX();

    // test::TestResult test_NOP_impl();

    // test::TestResult test_ORA_Xind();
    // test::TestResult test_ORA_abs();
    // test::TestResult test_ORA_absX();
    // test::TestResult test_ORA_absY();
    // test::TestResult test_ORA_imm();
    // test::TestResult test_ORA_indY();
    // test::TestResult test_ORA_zpg();
    // test::TestResult test_ORA_zpgX();

    // test::TestResult test_PHA_impl();

    // test::TestResult test_PHP_impl();

    // test::TestResult test_PLA_impl();

    // test::TestResult test_PLP_impl();

    // test::TestResult test_ROL_A();
    // test::TestResult test_ROL_abs();
    // test::TestResult test_ROL_absX();
    // test::TestResult test_ROL_zpg();
    // test::TestResult test_ROL_zpgX();

    // test::TestResult test_ROR_A();
    // test::TestResult test_ROR_abs();
    // test::TestResult test_ROR_absX();
    // test::TestResult test_ROR_zpg();
    // test::TestResult test_ROR_zpgX();

    // test::TestResult test_RTI_impl();

    // test::TestResult test_RTS_impl();

    // test::TestResult test_SBC_Xind();
    // test::TestResult test_SBC_abs();
    // test::TestResult test_SBC_absX();
    // test::TestResult test_SBC_absY();
    // test::TestResult test_SBC_imm();
    // test::TestResult test_SBC_indY();
    // test::TestResult test_SBC_zpg();
    // test::TestResult test_SBC_zpgX();

    // test::TestResult test_SEC_impl();

    // test::TestResult test_SED_impl();

    // test::TestResult test_SEI_impl();

    // test::TestResult test_STA_Xind();
    // test::TestResult test_STA_abs();
    // test::TestResult test_STA_absX();
    // test::TestResult test_STA_absY();
    // test::TestResult test_STA_indY();
    // test::TestResult test_STA_zpg();
    // test::TestResult test_STA_zpgX();

    // test::TestResult test_STX_abs();
    // test::TestResult test_STX_zpg();
    // test::TestResult test_STX_zpgY();

    // test::TestResult test_STY_abs();
    // test::TestResult test_STY_zpg();
    // test::TestResult test_STY_zpgX();

    // test::TestResult test_TAX_impl();

    // test::TestResult test_TAY_impl();

    // test::TestResult test_TSX_impl();

    // test::TestResult test_TXA_impl();

    // test::TestResult test_TXS_impl();

    // test::TestResult test_TYA_impl();
}
#endif