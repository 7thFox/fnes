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

    test::TestResult test_abs(
        uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_16([&](uint16_t abs_addr) {
                uint8_t value = std::rand() % 0xFF;
                std::unordered_map<uint16_t, uint8_t> mem = {
                    {0xFFFC, opcode},
                    {0xFFFD, abs_addr & 0xFF},
                    {0xFFFE, abs_addr >> 8},
                    {abs_addr, value},
                };
                return test_inst(test_name, &mem, cycles, std::bind(is_passed, std::placeholders::_1, value), cpu_setup);
            },0x0000, 0xFFFB);
        }
    test::TestResult test_absX(
        uint8_t opcode, std::string test_name, int cycles, int cycles_boundry,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_8([&](uint8_t x) {
                uint8_t value = std::rand() % 0xFF;
                uint16_t abs_addr = std::rand() % 0xFEFC;// addr + 0xFF < 0xFFFC
                std::unordered_map<uint16_t, uint8_t> mem = {
                    {0xFFFC, opcode},
                    {0xFFFD, abs_addr & 0xFF},
                    {0xFFFE, abs_addr >> 8},
                    {abs_addr + x, value},
                };
                return test_inst(test_name, &mem,
                    (abs_addr + x) & 0xFF00 != (abs_addr & 0xFF00) ? cycles_boundry : cycles,
                    std::bind(is_passed, std::placeholders::_1, value),
                    [&](components::Cpu6502 *cpu) {
                        if (cpu_setup) {
                            cpu_setup(cpu);
                        }
                        cpu->test_set_x(x);
                    });
            });
        }
    test::TestResult test_absY(
        uint8_t opcode, std::string test_name, int cycles, int cycles_boundry,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_8([&](uint8_t y) {
                uint8_t value = std::rand() % 0xFF;
                uint16_t abs_addr = std::rand() % 0xFEFC;// addr + 0xFF < 0xFFFC
                std::unordered_map<uint16_t, uint8_t> mem = {
                    {0xFFFC, opcode},
                    {0xFFFD, abs_addr & 0xFF},
                    {0xFFFE, abs_addr >> 8},
                    {abs_addr + y, value},
                };
                return test_inst(test_name, &mem,
                    (abs_addr + y) & 0xFF00 != (abs_addr & 0xFF00) ? cycles_boundry : cycles,
                    std::bind(is_passed, std::placeholders::_1, value),
                    [&](components::Cpu6502 *cpu) {
                        if (cpu_setup) {
                            cpu_setup(cpu);
                        }
                        cpu->test_set_y(y);
                    });
            });
        }
    test::TestResult test_imm(
        uint8_t opcode, std::string test_name,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_8([&](uint8_t value) {
                std::unordered_map<uint16_t, uint8_t> mem = {
                    {0xFFFC, opcode},
                    {0xFFFD, value},
                };
                return test_inst(test_name, &mem, 2, std::bind(is_passed, std::placeholders::_1, value), cpu_setup);
            });
        }
    test::TestResult test_ind(
        uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_8([&](uint8_t ind_hi) {
                return test_range_8([&](uint8_t ind_lo) {
                    uint8_t value = std::rand() % 0xFF;
                    uint16_t fetch_addr = std::rand() % 0xFFFA;
                    std::unordered_map<uint16_t, uint8_t> mem = {
                        {0xFFFC, opcode},
                        {0xFFFD, ind_lo},
                        {0xFFFE, ind_hi},
                        {ind_lo | (ind_hi << 8), fetch_addr & 0xFF},
                        {((ind_lo + 1) & 0xFF) | (ind_hi << 8), fetch_addr >> 8},
                        {fetch_addr, value},
                    };
                    return test_inst(test_name, &mem, cycles, std::bind(is_passed, std::placeholders::_1, value), cpu_setup);
                }, 0x00, ind_hi == 0xFF ? 0xFA : 0xFF);
            });
        }
    test::TestResult test_Xind(
        uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_8([&](uint8_t ind_zpg) {
                return test_range_8([&](uint8_t x) {
                    uint8_t value = std::rand() % 0xFF;
                    uint16_t fetch_addr = (std::rand() % 0xFEFA) + 0x100;// Keep out of zeropage to prevent conflicts
                    std::unordered_map<uint16_t, uint8_t> mem = {
                        {0xFFFC, opcode},
                        {0xFFFD, ind_zpg},
                        {(ind_zpg + x) & 0xFF, fetch_addr & 0xFF},
                        {((ind_zpg + x + 1) & 0xFF), fetch_addr >> 8},
                        {fetch_addr, value},
                    };
                    return test_inst(test_name, &mem, cycles, 
                        std::bind(is_passed, std::placeholders::_1, value),
                        [&](components::Cpu6502 *cpu) {
                            if (cpu_setup) {
                                cpu_setup(cpu);
                            }
                            cpu->test_set_x(x);
                        });
                });
            });
        }
    test::TestResult test_indY(
        uint8_t opcode, std::string test_name, int cycles, int cycles_boundry,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_8([&](uint8_t ind_zpg) {
                return test_range_8([&](uint8_t y) {
                    uint8_t value = std::rand() % 0xFF;
                    uint16_t fetch_addr = (std::rand() % 0xFDFC) + 0x100;// addr + 0xFF < 0xFFFC and keep out of zeropage
                    std::unordered_map<uint16_t, uint8_t> mem = {
                        {0xFFFC, opcode},
                        {0xFFFD, ind_zpg},
                        {ind_zpg, fetch_addr & 0xFF},
                        {((ind_zpg + 1) & 0xFF), fetch_addr >> 8},
                        {fetch_addr + y, value},
                    };
                    return test_inst(test_name, &mem,
                        (fetch_addr + y) & 0xFF00 != (fetch_addr & 0xFF00) ? cycles_boundry : cycles,
                        std::bind(is_passed, std::placeholders::_1, value),
                        [&](components::Cpu6502 *cpu) {
                            if (cpu_setup) {
                                cpu_setup(cpu);
                            }
                            cpu->test_set_y(y);
                        });
                });
            });
        }
    test::TestResult test_zpg(uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_8([&](uint8_t value) {
                return test_range_8([&](uint8_t zpg) {
                    std::unordered_map<uint16_t, uint8_t> mem = {
                        {0xFFFC, opcode},
                        {0xFFFD, zpg},
                        {zpg, value},
                    };
                    return test_inst(test_name, &mem, cycles, std::bind(is_passed, std::placeholders::_1, value), cpu_setup);
                });
            });
        }
    test::TestResult test_zpgX(uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_8([&](uint8_t zpg) {
                return test_range_8([&](uint8_t x) {
                    uint8_t value = std::rand() % 0xFF;
                    std::unordered_map<uint16_t, uint8_t> mem = {
                        {0xFFFC, opcode},
                        {0xFFFD, zpg},
                        {zpg + x & 0x00FF, value},
                    };
                    return test_inst(test_name, &mem, cycles,
                        std::bind(is_passed, std::placeholders::_1, value),
                        [&](components::Cpu6502 *cpu) {
                            if (cpu_setup) {
                                cpu_setup(cpu);
                            }
                            cpu->test_set_x(x);
                        });
                });
            });
        }
    test::TestResult test_zpgY(uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_8([&](uint8_t zpg) {
                return test_range_8([&](uint8_t y) {
                    uint8_t value = std::rand() % 0xFF;
                    std::unordered_map<uint16_t, uint8_t> mem = {
                        {0xFFFC, opcode},
                        {0xFFFD, zpg},
                        {zpg + y & 0x00FF, value},
                    };
                    return test_inst(test_name, &mem, cycles,
                        std::bind(is_passed, std::placeholders::_1, value),
                        [&](components::Cpu6502 *cpu) {
                            if (cpu_setup) {
                                cpu_setup(cpu);
                            }
                            cpu->test_set_y(y);
                        });
                });
            });
        }

    void test_ADC(test::TestSummary *summary)
    {
        summary->group_name = "ADC";
        summary->total = 7;

        auto is_passed = [](components::Cpu6502 *cpu, uint8_t value, uint8_t a0) {
            uint16_t sum = (uint16_t)value + (uint16_t)a0;
            return cpu->get_a() == (uint8_t)sum 
                && check_flag(cpu, components::Cpu6502Flags::N, ((uint8_t)sum & 0x80) == 0x80) 
                && check_flag(cpu, components::Cpu6502Flags::Z, (uint8_t)sum == 0) 
                && check_flag(cpu, components::Cpu6502Flags::C, sum > 0xFF) 
                && check_flag(cpu, components::Cpu6502Flags::V, ((~((uint16_t)a0 ^ (uint16_t)value) & ((uint16_t)a0 ^ sum)) & 0x0080) == 0x0080);
        };

        auto cpu_setup = [](components::Cpu6502 *cpu, uint8_t a0) {
            cpu->test_set_a(a0);
        };

        run_test(summary, [&]() {
            return test_range_8([&](uint8_t a0) {
                return test_imm(0x69, "ADC imm", 
                    std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0), 
                    std::bind(cpu_setup, std::placeholders::_1, a0));
            });
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_abs(0x6D, "ADC abs", 4, 
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0), 
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_absX(0x7D, "ADC abs,X", 4, 5, 
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0), 
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_absY(0x79, "ADC abs,Y", 4, 5, 
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0), 
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
    }
   
    void test_LDA(test::TestSummary *summary)
    {
        summary->group_name = "LDA";
        summary->total = 8;

        auto is_passed = [](components::Cpu6502 *cpu, uint8_t value) {
            return cpu->get_a() == value
                && check_flag(cpu, components::Cpu6502Flags::N, (value & 0x80) == 0x80)
                && check_flag(cpu, components::Cpu6502Flags::Z, value == 0);
        };

        run_test(summary, [&]() {
            return test_imm(0xA9, "LDA imm", is_passed);
        });
        run_test(summary, [&]() {
            return test_abs(0xAD, "LDA abs", 4, is_passed);
        });
        run_test(summary, [&]() {
            return test_absX(0xBD, "LDA abs,X", 4, 5, is_passed);
        });
        run_test(summary, [&]() {
            return test_absY(0xB9, "LDA abs,Y", 4, 5, is_passed);
        });
        run_test(summary, [&]() {
            return test_zpg(0xA5, "LDA zpg", 3, is_passed);
        });
        run_test(summary, [&]() {
            return test_zpgX(0xB5, "LDA zpg,X", 4, is_passed);
        });
        run_test(summary, [&]() {
            return test_Xind(0xA1, "LDA X,ind", 6, is_passed);
        });
        run_test(summary, [&]() {
            return test_indY(0xB1, "LDA ind,Y", 5, 6, is_passed);
        });
    }

    void run_all_instruction_tests(std::ostream*out, bool results_only){
        auto groups = {
            &test_ADC,
            &test_LDA,
        };

        for (auto test_group = groups.begin(); test_group != groups.end(); ++test_group)
        {
            test::TestSummary summary = test::TestSummary(out, results_only);
            (*test_group)(&summary);
            *out << std::endl;
            summary.write_summary(*out);
        }

        *out << std::endl
             << "All Tests Complete." << std::endl;
    }

    void run_test(test::TestSummary* summary, std::function<test::TestResult()> test_fn)
    {
        if (!summary->results_only)
        {
            *summary->out << "Testing " << (summary->failed + summary->passed + summary->partial_pass + 1) << "/" << summary->total << "..." << std::flush;
        }
        auto start = std::chrono::high_resolution_clock::now();
        auto result = test_fn();
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<double, std::ratio<1>>(end - start).count();
        summary->elapsed_time += elapsed;
        if (!summary->results_only)
        {
            *summary->out << "\r";
        }

        std::string result_name;
        if (!result.is_passed)
        {
            result_name = "Failed";
            summary->failed++;
        }
        else if (result.is_partial_fail){
            result_name = "Partial Pass";
            summary->partial_pass++;
        }
        else{
            result_name = "Passed";
            summary->passed++;
        }

        *summary->out << result.test_name << ": " << result_name;
        if (!summary->results_only)
        {
            *summary->out << " (" << std::fixed << std::setprecision(0) << elapsed * 1000 << " ms)";
        }
        if (!result.is_passed || result.is_partial_fail){
            *summary->out << " - " << result.message;
        }
        *summary->out << std::endl;
    }
}