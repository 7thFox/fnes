#include "tests_instructions.h"

namespace test
{

#define TEST_MAX_CYCLES 20
    test::TestResult test_inst(
        std::string test_name,
        std::unordered_map<uint16_t, uint8_t> *mem,
        int cycles, std::function<bool(components::Cpu6502 *)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup)
    {
        uint16_t address_bus;
        uint8_t data_bus;
        auto cpu = new components::Cpu6502(&address_bus, &data_bus);
        cpu->test_setup();
        if (cpu_setup)
        {
            cpu_setup(cpu);
        }
        cpu->test_start();
        int cycle_count = 0;
        while (cycle_count < TEST_MAX_CYCLES && !cpu->test_finished())
        {
            if (mem->count(address_bus) > 0)// Set data for given addr
            {
                data_bus = mem->at(address_bus);
            }
            cpu->set_clk();
            cycle_count++;
        }

        test::TestResult result;
        if (cycle_count < TEST_MAX_CYCLES)
        {            
            result.test_name = test_name;
            result.is_passed = is_passed(cpu);
            result.is_partial_fail = result.is_passed && (cycle_count != cycles);
            result.message = 
                !result.is_passed ? "CPU had unexpected values at the end of processing" : "Test Passed";
            if (result.is_partial_fail)
            {
                char buff[256];
                sprintf(buff, "Instruction completed correctly with incorrect cycle count. Wanted %d. Got %d.", cycles, cycle_count);
                result.message = buff;
            }
        }
        else
        {
            result.test_name = test_name;
            result.is_passed = false;
            result.is_partial_fail = false;
            result.message = "Took more than max allotted cycles to test";
        }

        delete cpu;

        return result;
    }
    
    test::TestResult test_range_8(std::function<test::TestResult(uint8_t)> test_fn, uint8_t lo, uint8_t hi)
    {
        test::TestResult res;
        test::TestResult res_partial_fail;
        res_partial_fail.is_partial_fail = false;
        for (int i = lo; i <= hi; i++)
        {
            res = test_fn((uint8_t)i);
            if (!res.is_passed){
                res.message += " param = " + std::to_string(i);
                return res;
            }
            else if (!res_partial_fail.is_partial_fail && res.is_partial_fail)
            {
                res.message += " param = " + std::to_string(i);
                res_partial_fail = res;
            }
        }
        if (res_partial_fail.is_partial_fail)
        {
            return res_partial_fail;
        }
        return res;
    }

    test::TestResult test_range_16(std::function<test::TestResult(uint16_t)> test_fn, uint16_t lo, uint16_t hi)
    {
        test::TestResult res;
        test::TestResult res_partial_fail;
        res_partial_fail.is_partial_fail = false;
        for (int i = lo; i <= hi; i++)
        {
            res = test_fn((uint16_t)i);
            if (!res.is_passed){
                return res;
            }
            else if (!res_partial_fail.is_partial_fail && res.is_partial_fail)
            {
                res_partial_fail = res;
            }
        }
        if (res_partial_fail.is_partial_fail){
            return res_partial_fail;
        }
        return res;
    }

    bool check_flag(components::Cpu6502 *cpu, components::Cpu6502Flags flg, bool set)
    {
        return set == ((cpu->get_p() & flg) == flg);
    }

    // test::TestResult test_ADC_Xind();
    // test::TestResult test_ADC_abs();
    // test::TestResult test_ADC_absX();
    // test::TestResult test_ADC_absY();
    // test::TestResult test_ADC_imm()
    // {
    //     return test_range_8([](uint8_t x) {
    //         std::unordered_map<uint16_t, uint8_t> mem = {
    //             {0xFFFC, 0x69},
    //             {0xFFFD, x},
    //         };
    //         return test_inst("test_ADC_imm", &mem, 2, [&](components::Cpu6502 *cpu) {
    //             return cpu->get_a() == (x + 0x52);
    //         }, [](components::Cpu6502 *cpu) {
    //             cpu->test_set_a(0x52);
    //         });
    //     });
    // }
    // test::TestResult test_ADC_indY();
    // test::TestResult test_ADC_zpg();
    // test::TestResult test_ADC_zpgX();

    test::TestResult test_LDA_Xind()
    {
        return test_range_8([](uint8_t x) {
            uint8_t val = x % 7 == 0 ? 0xB5 : // neg 
                x % 2 ? 0x00 : // zero
                0x13;// pos
            std::unordered_map<uint16_t, uint8_t> mem = {
                {0xFFFC, 0xA1},
                {0xFFFD, 0x53},
                {(0x0053 + x) & 0xFF, 0x84},
                {(0x0053 + x + 1) & 0xFF, 0x21},
                {0x2184, val}
            };
            return test_inst(
                "test_LDA_Xind", &mem, 6,
                [&](components::Cpu6502 *cpu) {
                    return cpu->get_a() == val
                        && check_flag(cpu, components::Cpu6502Flags::N, val & 0x80 == 0x80)
                        && check_flag(cpu, components::Cpu6502Flags::Z, val == 0);
                },
                [&](components::Cpu6502 *cpu) {
                    cpu->test_set_x(x);
                });
        });
    }
    test::TestResult test_LDA_abs()
    {
        return test_range_16([](uint16_t x) {
            uint8_t val = x % 7 == 0 ? 0xEE : // neg 
                x % 2 ? 0x00 : // zero
                0x69;// pos
            std::unordered_map<uint16_t, uint8_t> mem = 
            {
                {0xFFFC, 0xAD},
                {0xFFFD, x & 0xFF},
                {0xFFFE, x >> 8},
                {x, val},
            };
            return test_inst("test_LDA_abs", &mem, 4, [&](components::Cpu6502 *cpu) {
                return cpu->get_a() == val
                    && check_flag(cpu, components::Cpu6502Flags::N, val & 0x80 == 0x80)
                    && check_flag(cpu, components::Cpu6502Flags::Z, val == 0);
            });
        }, 0x6748, 0x6948);
    }
    test::TestResult test_LDA_absX()
    {
        return test_range_8([](uint8_t x) {
            uint8_t val = x % 7 == 0 ? 0xF0 : // neg 
                x % 2 ? 0x00 : // zero
                0x21;// pos
            std::unordered_map<uint16_t, uint8_t> mem = {
                {0xFFFC, 0xBD},
                {0xFFFD, 0x74},
                {0xFFFE, 0x84},
                {0x8474 + x, val},
            };
            return test_inst(
                "test_LDA_absX", &mem,
                (0x8474 + x) & 0xFF00 != 0x8400 ? 5 : 4, // add for page boundry
                [&](components::Cpu6502 *cpu) {
                    return cpu->get_a() == val
                        && check_flag(cpu, components::Cpu6502Flags::N, val & 0x80 == 0x80)
                        && check_flag(cpu, components::Cpu6502Flags::Z, val == 0);
                },
                [&](components::Cpu6502 *cpu) {
                    cpu->test_set_x(x);
                });
        });
    }
    test::TestResult test_LDA_absY()
    {
        return test_range_8([](uint8_t x) {
            uint8_t val = x % 7 == 0 ? 0x88 : // neg 
                x % 2 ? 0x00 : // zero
                0x08;// pos
            std::unordered_map<uint16_t, uint8_t> mem = {
                {0xFFFC, 0xB9},
                {0xFFFD, 0x99},
                {0xFFFE, 0x66},
                {0x6699 + x, val},
            };

            return test_inst(
                "test_LDA_absY", &mem,
                (0x6699 + x) & 0xFF00 != 0x6600 ? 5 : 4, // add for page boundry
                [&](components::Cpu6502 *cpu) {
                    return cpu->get_a() == val
                        && check_flag(cpu, components::Cpu6502Flags::N, val & 0x80 == 0x80)
                        && check_flag(cpu, components::Cpu6502Flags::Z, val == 0);
                },
                [&](components::Cpu6502 *cpu) {
                    cpu->test_set_y(x);
                });
        });
    }
    test::TestResult test_LDA_imm()
    {
        return test_range_8([](uint8_t x) {
            std::unordered_map<uint16_t, uint8_t> mem = {
                {0xFFFC, 0xA9},
                {0xFFFD, x},
            };
            return test_inst("test_LDA_imm", &mem, 2, [&](components::Cpu6502 *cpu) {
                return cpu->get_a() == x
                    && check_flag(cpu, components::Cpu6502Flags::N, x & 0x80 == 0x80)
                    && check_flag(cpu, components::Cpu6502Flags::Z, x == 0);
            });
        });
    }
    test::TestResult test_LDA_indY()
    {
        return test_range_8([](uint8_t x) {
            uint8_t val = x % 7 == 0 ? 0x9E : // neg 
                x % 2 ? 0x00 : // zero
                0x33;// pos
            std::unordered_map<uint16_t, uint8_t> mem = {
                {0xFFFC, 0xB1},
                {0xFFFD, 0x79},
                {(0x0079) & 0xFF, 0xC3},
                {(0x0079 + 1) & 0xFF, 0x44},
                {0x44C3 + x, val}
            };
            return test_inst(
                "test_LDA_indY", &mem, 
                (0x44C3 + x) & 0xFF00 != 0x4400 ? 6 : 5,
                [&](components::Cpu6502 *cpu) {
                    return cpu->get_a() == val
                        && check_flag(cpu, components::Cpu6502Flags::N, val & 0x80 == 0x80)
                        && check_flag(cpu, components::Cpu6502Flags::Z, val == 0);
                },
                [&](components::Cpu6502 *cpu) {
                    cpu->test_set_y(x);
                });
        });
    }
    test::TestResult test_LDA_zpg()
    {
        return test_range_8([&](uint8_t x) 
        {
            uint8_t val = x % 7 == 0 ? 0xCF : // neg 
                x % 2 ? 0x00 : // zero
                0x38;// pos
            std::unordered_map<uint16_t, uint8_t> mem = {
                {0xFFFC, 0xA5},
                {0xFFFD, x},
                {x, val},
            };
            return test_inst("test_LDA_zpg", &mem, 3, [&](components::Cpu6502 *cpu) {
                return cpu->get_a() == val
                    && check_flag(cpu, components::Cpu6502Flags::N, val & 0x80 == 0x80)
                    && check_flag(cpu, components::Cpu6502Flags::Z, val == 0);
            });
        });
    }
    test::TestResult test_LDA_zpgX()
    {
        return test_range_8([](uint8_t x) {
            uint8_t val = x % 7 == 0 ? 0xFD : // neg 
                x % 2 ? 0x00 : // zero
                0x51;// pos
            std::unordered_map<uint16_t, uint8_t> mem = {
                {0xFFFC, 0xB5},
                {0xFFFD, 0x37},
                {((0x0037 + x) & 0xFF), val},
            };
            return test_inst(
                "test_LDA_zpgX", &mem, 4,
                [&](components::Cpu6502 *cpu) {
                    return cpu->get_a() == val
                        && check_flag(cpu, components::Cpu6502Flags::N, val & 0x80 == 0x80)
                        && check_flag(cpu, components::Cpu6502Flags::Z, val == 0);
                },
                [&](components::Cpu6502 *cpu) {
                    cpu->test_set_x(x);
                });
        });
    }

    void run_all_instruction_tests(std::ostream& out, bool results_only){
        auto tests = {
            // &test_ADC_Xind,
            // &test_ADC_abs,
            // &test_ADC_absX,
            // &test_ADC_absY,
            // &test_ADC_imm,
            // &test_ADC_indY,
            // &test_ADC_zpg,
            // &test_ADC_zpgX,

            // &test_AND_Xind,
            // &test_AND_abs,
            // &test_AND_absX,
            // &test_AND_absY,
            // &test_AND_imm,
            // &test_AND_indY,
            // &test_AND_zpg,
            // &test_AND_zpgX,

            // &test_ASL_A,
            // &test_ASL_abs,
            // &test_ASL_absX,
            // &test_ASL_zpg,
            // &test_ASL_zpgX,

            // &test_BCC_rel,

            // &test_BCS_rel,

            // &test_BEQ_rel,

            // &test_BIT_abs,
            // &test_BIT_zpg,

            // &test_BMI_rel,

            // &test_BNE_rel,

            // &test_BPL_rel,

            // &test_BRK_impl,

            // &test_BVC_rel,

            // &test_BVS_rel,

            // &test_CLC_impl,

            // &test_CLD_impl,

            // &test_CLI_impl,

            // &test_CLV_impl,

            // &test_CMP_Xind,
            // &test_CMP_abs,
            // &test_CMP_absX,
            // &test_CMP_absY,
            // &test_CMP_imm,
            // &test_CMP_indY,
            // &test_CMP_zpg,
            // &test_CMP_zpgX,

            // &test_CPX_abs,
            // &test_CPX_imm,
            // &test_CPX_zpg,

            // &test_CPY_abs,
            // &test_CPY_imm,
            // &test_CPY_zpg,

            // &test_DEC_abs,
            // &test_DEC_absX,
            // &test_DEC_zpg,
            // &test_DEC_zpgX,

            // &test_DEX_impl,

            // &test_DEY_impl,

            // &test_EOR_Xind,
            // &test_EOR_abs,
            // &test_EOR_absX,
            // &test_EOR_absY,
            // &test_EOR_imm,
            // &test_EOR_indY,
            // &test_EOR_zpg,
            // &test_EOR_zpgX,

            // &test_INC_abs,
            // &test_INC_absX,
            // &test_INC_zpg,
            // &test_INC_zpgX,
            // &test_INX_impl,
            // &test_INY_impl,

            // &test_JMP_abs,
            // &test_JMP_ind,

            // &test_JSR_abs,

            &test_LDA_Xind,
            &test_LDA_abs,
            &test_LDA_absX,
            &test_LDA_absY,
            &test_LDA_imm,
            &test_LDA_indY,
            &test_LDA_zpg,
            &test_LDA_zpgX,

            // &test_LDX_abs,
            // &test_LDX_absY,
            // &test_LDX_imm,
            // &test_LDX_zpg,
            // &test_LDX_zpgY,

            // &test_LDY_abs,
            // &test_LDY_absX,
            // &test_LDY_imm,
            // &test_LDY_zpg,
            // &test_LDY_zpgX,

            // &test_LSR_A,
            // &test_LSR_abs,
            // &test_LSR_absX,
            // &test_LSR_zpg,
            // &test_LSR_zpgX,

            // &test_NOP_impl,

            // &test_ORA_Xind,
            // &test_ORA_abs,
            // &test_ORA_absX,
            // &test_ORA_absY,
            // &test_ORA_imm,
            // &test_ORA_indY,
            // &test_ORA_zpg,
            // &test_ORA_zpgX,

            // &test_PHA_impl,

            // &test_PHP_impl,

            // &test_PLA_impl,

            // &test_PLP_impl,

            // &test_ROL_A,
            // &test_ROL_abs,
            // &test_ROL_absX,
            // &test_ROL_zpg,
            // &test_ROL_zpgX,

            // &test_ROR_A,
            // &test_ROR_abs,
            // &test_ROR_absX,
            // &test_ROR_zpg,
            // &test_ROR_zpgX,

            // &test_RTI_impl,

            // &test_RTS_impl,

            // &test_SBC_Xind,
            // &test_SBC_abs,
            // &test_SBC_absX,
            // &test_SBC_absY,
            // &test_SBC_imm,
            // &test_SBC_indY,
            // &test_SBC_zpg,
            // &test_SBC_zpgX,

            // &test_SEC_impl,

            // &test_SED_impl,

            // &test_SEI_impl,

            // &test_STA_Xind,
            // &test_STA_abs,
            // &test_STA_absX,
            // &test_STA_absY,
            // &test_STA_indY,
            // &test_STA_zpg,
            // &test_STA_zpgX,

            // &test_STX_abs,
            // &test_STX_zpg,
            // &test_STX_zpgY,

            // &test_STY_abs,
            // &test_STY_zpg,
            // &test_STY_zpgX,

            // &test_TAX_impl,

            // &test_TAY_impl,

            // &test_TSX_impl,

            // &test_TXA_impl,

            // &test_TXS_impl,

            // &test_TYA_impl,
        };

        int passed = 0;
        int partial_fail = 0;
        int failed = 0;
        int count = 0;
        for (auto test_fn = tests.begin(); test_fn != tests.end(); ++test_fn)
        {
            if (!results_only)
            {
                out << "Testing " << ++count << "/" << tests.size() << "..." << std::flush;
            }
            auto result = (*test_fn)();
            if (!results_only){
                out << "\r";
            }

            std::string result_name;
            if (!result.is_passed)
            {
                result_name = "Failed";
                failed++;
            }
            else if (result.is_partial_fail){
                result_name = "Partial Fail";
                partial_fail++;
            }
            else{
                result_name = "Passed";
                passed++;
            }

            out << result.test_name << ": " << result_name;
            if (!result.is_passed || result.is_partial_fail){
                out << " - " << result.message;
            }
            out << std::endl;
        }

        out << std::endl
            << "--------------Test Summary--------------" << std::endl
            << " Success: " << passed << std::endl
            << " Failed: " << failed << std::endl
            << " Partial Fail: " << partial_fail << std::endl;
    }
}