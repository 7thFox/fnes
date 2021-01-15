#ifndef TESTS_INSTRUCTIONS_H
#define TESTS_INSTRUCTIONS_H

#include "test.h"
#include "components/cpu6502.h"
#include <stdint.h>
#include <functional>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstdlib>

namespace test
{
    void run_all_instruction_tests(std::ostream *out, bool results_only);

    test::TestResult test_inst(
        std::string test_name,
        std::unordered_map<uint16_t, uint8_t> *mem,
        int cycles, std::function<bool(components::Cpu6502 *)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup = {});

    test::TestResult test_range_8(std::function<test::TestResult(uint8_t)> test_fn, uint8_t lo = 0x00, uint8_t hi = 0xFF);
    test::TestResult test_range_16(std::function<test::TestResult(uint16_t)> test_fn, uint16_t lo = 0x0000, uint16_t hi = 0xFFFF);

    bool check_flag(components::Cpu6502 *cpu, components::Cpu6502Flags flg, bool set);

    test::TestResult test_A(
        uint8_t opcode, std::string test_name,
        std::function<bool(components::Cpu6502 *, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup = {});
    test::TestResult test_abs(
        uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502 *, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup = {});
    test::TestResult test_absX(
        uint8_t opcode, std::string test_name, int cycles, int cycles_boundry,
        std::function<bool(components::Cpu6502 *, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup = {});
    test::TestResult test_absY(
        uint8_t opcode, std::string test_name, int cycles, int cycles_boundry,
        std::function<bool(components::Cpu6502 *, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup = {});
    test::TestResult test_imm(
        uint8_t opcode, std::string test_name,
        std::function<bool(components::Cpu6502 *, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup = {});
    // test::TestResult test_impl
    test::TestResult test_ind(
        uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502 *, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup = {});
    test::TestResult test_Xind(
        uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502 *, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup = {});
    test::TestResult test_indY(
        uint8_t opcode, std::string test_name, int cycles, int cycles_boundry,
        std::function<bool(components::Cpu6502 *, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup = {});
    test::TestResult test_rel(
        uint8_t opcode, std::string test_name,
        std::function<bool(components::Cpu6502 *, uint16_t, int8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint16_t, int8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup = {});
    test::TestResult test_zpg(
        uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502 *, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup = {});
    test::TestResult test_zpgX(
        uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502 *, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup = {});
    test::TestResult test_zpgY(
        uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502 *, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup = {});

    void test_branch(test::TestSummary *summary, uint8_t opcode, std::string name, components::Cpu6502Flags flag, bool set);

    void run_test(test::TestSummary *summary, std::function<test::TestResult()> test_fn);
    void test_ADC(test::TestSummary *summary);
    void test_AND(test::TestSummary *summary);
    void test_ASL(test::TestSummary *summary);
    void test_BCC(test::TestSummary *summary);
    void test_BCS(test::TestSummary *summary);
    // void test_BEQ(test::TestSummary *summary);
    // void test_BIT(test::TestSummary *summary);
    // void test_BMI(test::TestSummary *summary);
    // void test_BNE(test::TestSummary *summary);
    // void test_BPL(test::TestSummary *summary);
    // void test_BRK(test::TestSummary *summary);
    // void test_BVC(test::TestSummary *summary);
    // void test_BVS(test::TestSummary *summary);
    // void test_CLC(test::TestSummary *summary);
    // void test_CLD(test::TestSummary *summary);
    // void test_CLI(test::TestSummary *summary);
    // void test_CLV(test::TestSummary *summary);
    // void test_CMP(test::TestSummary *summary);
    // void test_CPX(test::TestSummary *summary);
    // void test_CPY(test::TestSummary *summary);
    // void test_DEC(test::TestSummary *summary);
    // void test_DEX(test::TestSummary *summary);
    // void test_DEY(test::TestSummary *summary);
    // void test_EOR(test::TestSummary *summary);
    // void test_INC(test::TestSummary *summary);
    // void test_INX(test::TestSummary *summary);
    // void test_INY(test::TestSummary *summary);
    // void test_JMP(test::TestSummary *summary);
    // void test_JSR(test::TestSummary *summary);
    void test_LDA(test::TestSummary *summary);
    // void test_LDX(test::TestSummary *summary);
    // void test_LDY(test::TestSummary *summary);
    // void test_LSR(test::TestSummary *summary);
    // void test_NOP(test::TestSummary *summary);
    // void test_ORA(test::TestSummary *summary);
    // void test_PHA(test::TestSummary *summary);
    // void test_PHP(test::TestSummary *summary);
    // void test_PLA(test::TestSummary *summary);
    // void test_PLP(test::TestSummary *summary);
    // void test_ROL(test::TestSummary *summary);
    // void test_ROR(test::TestSummary *summary);
    // void test_RTI(test::TestSummary *summary);
    // void test_RTS(test::TestSummary *summary);
    // void test_SBC(test::TestSummary *summary);
    // void test_SEC(test::TestSummary *summary);
    // void test_SED(test::TestSummary *summary);
    // void test_SEI(test::TestSummary *summary);
    // void test_STA(test::TestSummary *summary);
    // void test_STX(test::TestSummary *summary);
    // void test_STY(test::TestSummary *summary);
    // void test_TAX(test::TestSummary *summary);
    // void test_TAY(test::TestSummary *summary);
    // void test_TSX(test::TestSummary *summary);
    // void test_TXA(test::TestSummary *summary);
    // void test_TXS(test::TestSummary *summary);
    // void test_TYA(test::TestSummary *summary);
}
#endif