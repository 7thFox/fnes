#include "tests_instructions.h"

namespace test
{

#define TEST_MAX_CYCLES 20
    test::TestResult test_inst(
        std::string test_name,
        std::unordered_map<uint16_t, uint8_t> *mem,
        int cycles, std::function<bool(components::Cpu6502 *)> is_passed,
        std::function<void(char*, components::Cpu6502 *)> print_debug,
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
            char buff[256];
            if (result.is_partial_fail)
            {
                sprintf(buff, "Instruction completed correctly with incorrect cycle count. Wanted %d. Got %d.", cycles, cycle_count);
                result.message = buff;
            }
            if (!result.is_passed || result.is_partial_fail)
            {
                print_debug(buff, cpu);
                result.message += std::string(buff);
            }
        }
        else
        {
            result.test_name = test_name;
            result.is_passed = false;
            result.is_partial_fail = false;
            result.message = "Took more than max allotted cycles to test";
            char buff[256];
            print_debug(buff, cpu);
            result.message += std::string(buff);
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
                return res;
            }
            else if (!res_partial_fail.is_partial_fail && res.is_partial_fail)
            {
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
    test::TestResult test_A(
        uint8_t opcode, std::string test_name,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_8([&](uint8_t a) {
                std::unordered_map<uint16_t, uint8_t> mem = {
                    {0xFFFC, opcode},
                };
                return test_inst(test_name, &mem, 2, 
                    std::bind(is_passed, std::placeholders::_1, a), 
                    std::bind(print_on_fail, std::placeholders::_1, std::placeholders::_2, a), 
                    [&](components::Cpu6502 *cpu) {
                        if (cpu_setup) {
                            cpu_setup(cpu);
                        }
                        cpu->test_set_a(a);
                    });
            });
        }
    test::TestResult test_abs(
        uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
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
                return test_inst(test_name, &mem, cycles, 
                    std::bind(is_passed, std::placeholders::_1, value), 
                    std::bind(print_on_fail, std::placeholders::_1, std::placeholders::_2, value), 
                    cpu_setup);
            },0x0000, 0xFFFB);
        }
    test::TestResult test_absX(
        uint8_t opcode, std::string test_name, int cycles, int cycles_boundry,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
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
                    std::bind(print_on_fail, std::placeholders::_1, std::placeholders::_2, value), 
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
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
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
                    std::bind(print_on_fail, std::placeholders::_1, std::placeholders::_2, value), 
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
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_8([&](uint8_t value) {
                std::unordered_map<uint16_t, uint8_t> mem = {
                    {0xFFFC, opcode},
                    {0xFFFD, value},
                };
                return test_inst(test_name, &mem, 2, 
                    std::bind(is_passed, std::placeholders::_1, value), 
                    std::bind(print_on_fail, std::placeholders::_1, std::placeholders::_2, value), 
                    cpu_setup);
            });
        }
    test::TestResult test_ind(
        uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
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
                    return test_inst(test_name, &mem, cycles, 
                        std::bind(is_passed, std::placeholders::_1, value),
                        std::bind(print_on_fail, std::placeholders::_1, std::placeholders::_2, value), 
                        cpu_setup);
                }, 0x00, ind_hi == 0xFF ? 0xFA : 0xFF);
            });
        }
    test::TestResult test_Xind(
        uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
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
                        std::bind(print_on_fail, std::placeholders::_1, std::placeholders::_2, value), 
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
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
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
                        std::bind(print_on_fail, std::placeholders::_1, std::placeholders::_2, value), 
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
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
        std::function<void(components::Cpu6502*)> cpu_setup)
        {
            return test_range_8([&](uint8_t value) {
                return test_range_8([&](uint8_t zpg) {
                    std::unordered_map<uint16_t, uint8_t> mem = {
                        {0xFFFC, opcode},
                        {0xFFFD, zpg},
                        {zpg, value},
                    };
                    return test_inst(test_name, &mem, cycles, 
                        std::bind(is_passed, std::placeholders::_1, value), 
                        std::bind(print_on_fail, std::placeholders::_1, std::placeholders::_2, value), 
                        cpu_setup);
                });
            });
        }
    test::TestResult test_rel(
        uint8_t opcode, std::string test_name,
        std::function<bool(components::Cpu6502 *,  uint16_t, int8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint16_t, int8_t)> print_on_fail,
        std::function<void(components::Cpu6502 *)> cpu_setup)
        {
            const uint16_t range = 0xFFFF - 0xFF - 0xFF - 1;
            uint16_t addr_origin = (std::rand() % range) + 0x100; // We wnat to ensure we test page boundries, so we'll use just one origin point
            return test_range_8([&](uint8_t val) {
                int8_t rel = val;
                std::unordered_map<uint16_t, uint8_t> mem = {
                    {addr_origin-1, opcode},
                    {addr_origin, rel},
                };
                auto cycles_page = (addr_origin + rel) & 0xFF00 == addr_origin & 0xFF00
                    ? 3
                    : 4;

                return test_inst(test_name, &mem, cycles_page, 
                    std::bind(is_passed, std::placeholders::_1, addr_origin, rel), 
                    std::bind(print_on_fail, std::placeholders::_1, std::placeholders::_2, addr_origin, rel), 
                    [&](components::Cpu6502 *cpu) {
                        if (cpu_setup) {
                            cpu_setup(cpu);
                        }
                        cpu->test_set_pc(addr_origin-1);
                    });
            });
        }
    test::TestResult test_zpgX(uint8_t opcode, std::string test_name, int cycles,
        std::function<bool(components::Cpu6502*, uint8_t)> is_passed,
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
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
                        std::bind(print_on_fail, std::placeholders::_1,  std::placeholders::_2, value),
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
        std::function<void(char*, components::Cpu6502 *, uint8_t)> print_on_fail,
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
                        std::bind(print_on_fail, std::placeholders::_1, std::placeholders::_2, value), 
                        [&](components::Cpu6502 *cpu) {
                            if (cpu_setup) {
                                cpu_setup(cpu);
                            }
                            cpu->test_set_y(y);
                        });
                });
            });
        }

    void run_all_instruction_tests(std::ostream*out, bool results_only){
        auto groups = {
            &test_ADC,
            &test_AND,
            &test_ASL,
            &test_BCC,
            &test_BCS,
            &test_BEQ,
            &test_BIT,
            &test_BMI,
            &test_BNE,
            // &test_BRK,
            &test_BPL,
            &test_BVC,
            &test_BVS,

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

    void test_branch(test::TestSummary *summary, uint8_t opcode, std::string name, components::Cpu6502Flags flag, bool on_set)
    {
        summary->group_name = name;
        summary->total = 2;

        run_test(summary, [&]() {
            return test_rel(opcode, name + " rel (Branch)", 
                [](components::Cpu6502 *cpu, uint16_t origin, int8_t rel) {
                    return cpu->get_pc() == origin + rel;
                },
                [&](char* buf, components::Cpu6502 *cpu, uint16_t origin, int8_t rel) {
                    sprintf(buf, "\n PC: 0x%04x + 0x%02x = 0x%04x =? 0x%04x Flag: %d\n",
                        origin & 0xFFFF, rel, (origin & 0xFFFF) + rel, cpu->get_pc(), cpu->get_p() & flag);
                }, 
                [&](components::Cpu6502 *cpu) {
                    cpu->test_set_p(on_set ? flag : ~flag);
                });
        });
        run_test(summary, [&]() {
            return test_rel(opcode, name + " rel (Nobranch)", 
                [](components::Cpu6502 *cpu, uint16_t origin, int8_t rel) {
                    return cpu->get_pc() == origin + 1;
                },
                [&](char* buf, components::Cpu6502 *cpu, uint16_t origin, int8_t rel) {
                    sprintf(buf, "\n PC: 0x%04x + 0x01 = 0x%04x =? 0x%04x Flag: %d\n",
                    origin & 0xFFFF, (origin & 0xFFFF) + 0x01, cpu->get_pc(), cpu->get_p() & flag);
                }, 
                [&](components::Cpu6502 *cpu) {
                    cpu->test_set_p(on_set ? ~flag : flag);
                });
        });
    }

    void run_test(test::TestSummary* summary, std::function<test::TestResult()> test_fn)
    {
        if (!summary->terse)
        {
            *summary->out << "Testing " << (summary->failed + summary->passed + summary->partial_pass + 1) << "/" << summary->total << "..." << std::flush;
        }
        auto start = std::chrono::high_resolution_clock::now();
        auto result = test_fn();
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<double, std::ratio<1>>(end - start).count();
        summary->elapsed_time += elapsed;

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

        if (!summary->terse)
        {
            *summary->out << "\r" << result.test_name << ": " << result_name << " (" << std::fixed << std::setprecision(0) << elapsed * 1000 << " ms)";

            if (!result.is_passed || result.is_partial_fail)
            {
                *summary->out << " - " << result.message;
            }
            *summary->out << std::endl;
        }
        else if (!result.is_passed || result.is_partial_fail)
        {
            *summary->out << std::endl
                          << result.test_name << ": Failed - " << result.message;
        }
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

        auto debug = [](char* buf, components::Cpu6502 *cpu, uint8_t value, uint8_t a0) {
            sprintf(buf, "A0: 0x%02 + $: 0x%02 = A: 0x%02 =? A: 0x%02 | N:%d Z:%d C:%d V:%d",
                   a0, value, (a0 + value) & 0xFF, cpu->get_a(),
                   cpu->get_p() & components::Cpu6502Flags::N,
                   cpu->get_p() & components::Cpu6502Flags::Z,
                   cpu->get_p() & components::Cpu6502Flags::C,
                   cpu->get_p() & components::Cpu6502Flags::V);
        };

        run_test(summary, [&]() {
            return test_range_8([&](uint8_t a0) {
                return test_imm(0x69, "ADC imm", 
                    std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                    std::bind(debug, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3, a0),
                    std::bind(cpu_setup, std::placeholders::_1, a0));
            });
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_abs(0x6D, "ADC abs", 4, 
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_absX(0x7D, "ADC abs,X", 4, 5, 
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_absY(0x79, "ADC abs,Y", 4, 5, 
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_zpg(0x65, "ADC zpg", 3,
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_zpgX(0x75, "ADC zpg,X", 4,
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_Xind(0x61, "ADC X,ind", 6,
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_indY(0x71, "ADC ind,Y", 5, 6,
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
    } 
    
    void test_AND(test::TestSummary *summary)
    {
        summary->group_name = "AND";
        summary->total = 8;

        auto is_passed = [](components::Cpu6502 *cpu, uint8_t value, uint8_t a0) {
            return cpu->get_a() == (value & a0)
                && check_flag(cpu, components::Cpu6502Flags::N, ((value & a0) & 0x80) == 0x80)
                && check_flag(cpu, components::Cpu6502Flags::Z, (value & a0) == 0);
        };
        auto cpu_setup = [](components::Cpu6502 *cpu, uint8_t a0) {
            cpu->test_set_a(a0);
        };
        auto debug = [](char* buf, components::Cpu6502 *cpu, uint8_t value, uint8_t a0) {
            sprintf(buf, "A0: 0x%02 & $: 0x%02 = A: 0x%02 =? A: 0x%02 | N:%d Z:%d",
                   a0, value, a0 & value, cpu->get_a(),
                   cpu->get_p() & components::Cpu6502Flags::N,
                   cpu->get_p() & components::Cpu6502Flags::Z);
        };

        
        run_test(summary, [&]() {
            return test_range_8([&](uint8_t a0) {
                return test_imm(0x29, "AND imm", 
                    std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                    std::bind(debug, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, a0),
                    std::bind(cpu_setup, std::placeholders::_1, a0));
            });
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_zpg(0x25, "AND zpg", 3,
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_zpgX(0x35, "AND zpg,X", 4,
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_abs(0x2D, "AND abs", 4, 
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_absX(0x3D, "AND abs,X", 4, 5, 
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_absY(0x39, "AND abs,Y", 4, 5, 
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_Xind(0x21, "AND X,ind", 6,
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
        run_test(summary, [&]() {
            uint8_t a0 = std::rand() & 0xFF;
            return test_indY(0x31, "AND ind,Y", 5, 6,
                std::bind(is_passed, std::placeholders::_1, std::placeholders::_2, a0),
                std::bind(debug, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, a0),
                std::bind(cpu_setup, std::placeholders::_1, a0));
        });
    }

    void test_ASL(test::TestSummary *summary)
    {
        summary->group_name = "ASL";
        summary->total = 8;

        auto is_passed = [](components::Cpu6502 *cpu, uint8_t value) {
            auto expected = (value << 1) & 0xFF;
            return cpu->get_a() == expected
                && check_flag(cpu, components::Cpu6502Flags::N, (expected & 0x80) == 0x80)
                && check_flag(cpu, components::Cpu6502Flags::Z, expected == 0)
                && check_flag(cpu, components::Cpu6502Flags::C, (value & 0x80) == 0x80);
        };

        auto debug = [](char* buf, components::Cpu6502 *cpu, uint8_t a0) {
            sprintf(buf, "A: 0x%02 << 1 = A: 0x%02 =? A: 0x%02 | N:%d Z:%d",
                a0, a0 << 1, cpu->get_a(),
                cpu->get_p() & components::Cpu6502Flags::N,
                cpu->get_p() & components::Cpu6502Flags::Z);
        };

        run_test(summary, [&]() {
            return test_A(0x0A, "ASL A", is_passed, debug);
        });
        run_test(summary, [&]() {
            return test_zpg(0x06, "ASL zpg", 5, is_passed, debug);
        });
        run_test(summary, [&]() {
            return test_zpgX(0x16, "ASL zpg,X", 6, is_passed, debug);
        });
        run_test(summary, [&]() {
            return test_abs(0x0E, "ASL abs", 6, is_passed, debug);
        });
        run_test(summary, [&]() {
            return test_absX(0x1E, "ASL abs,X", 7, 7, is_passed, debug);
        });
    }

    void test_BCC(test::TestSummary *summary)
    {
        test_branch(summary, 0x90, "BCC", components::Cpu6502Flags::C, false);
    }

    void test_BCS(test::TestSummary *summary)
    {
        test_branch(summary, 0xB0, "BCS", components::Cpu6502Flags::C, true);
    }
    
    void test_BEQ(test::TestSummary *summary)
    {
        test_branch(summary, 0xF0, "BEQ", components::Cpu6502Flags::Z, true);
    }

    void test_BIT(test::TestSummary *summary)
    {
        summary->group_name = "BIT";
        summary->total = 2;

        run_test(summary, []() {
            return test_range_8([](uint8_t a) {
                return test_zpg(
                    0x24, "BIT zpg", 3, [&](components::Cpu6502 *cpu, uint8_t m) {
                        return ((cpu->get_p() & components::Cpu6502Flags::N) == components::Cpu6502Flags::N) == ((m & (1 << 7)) == (1 << 7)) &&
                            ((cpu->get_p() & components::Cpu6502Flags::V) == components::Cpu6502Flags::V) == ((m & (1 << 6)) == (1 << 6)) &&
                            ((cpu->get_p() & components::Cpu6502Flags::Z) == components::Cpu6502Flags::Z) == ((m & a) == 0);
                    },
                    [&](char* buf, components::Cpu6502 *cpu, uint8_t m) {
                        sprintf(buf, "N: %d ?= %d V: %d ?= %d Z: %d ?= %d",
                            cpu->get_p() & components::Cpu6502Flags::N, m & (1 << 7),
                            cpu->get_p() & components::Cpu6502Flags::V, m & (1 << 6),
                            cpu->get_p() & components::Cpu6502Flags::Z, m & a);
                    },
                    [&](components::Cpu6502 *cpu) {
                        cpu->test_set_a(a);
                    });
            }, 0x00, 0x01);
        });

        run_test(summary, []() {
            return test_range_8([](uint8_t a) {
                return test_abs(
                    0x2C, "BIT abs", 4, [&](components::Cpu6502 *cpu, uint8_t m) {
                        return ((cpu->get_p() & components::Cpu6502Flags::N) == components::Cpu6502Flags::N) == ((m & (1 << 7)) == (1 << 7)) &&
                            ((cpu->get_p() & components::Cpu6502Flags::V) == components::Cpu6502Flags::V) == ((m & (1 << 6)) == (1 << 6)) &&
                            ((cpu->get_p() & components::Cpu6502Flags::Z) == components::Cpu6502Flags::Z) == ((m & a) == 0);
                    },
                    [&](char* buf, components::Cpu6502 *cpu, uint8_t m) {
                        sprintf(buf, "N: %d ?= %d V: %d ?= %d Z: %d ?= %d",
                            cpu->get_p() & components::Cpu6502Flags::N, m & (1 << 7),
                            cpu->get_p() & components::Cpu6502Flags::V, m & (1 << 6),
                            cpu->get_p() & components::Cpu6502Flags::Z, m & a);
                    },
                    [&](components::Cpu6502 *cpu) {
                        cpu->test_set_a(a);
                    });
            }, 0x00, 0x01);
        });
    }

    void test_BMI(test::TestSummary *summary)
    {
        test_branch(summary, 0x30, "BMI", components::Cpu6502Flags::N, true);
    }

    void test_BNE(test::TestSummary *summary)
    {
        test_branch(summary, 0xD0, "BNE", components::Cpu6502Flags::Z, false);
    }
    
    void test_BPL(test::TestSummary *summary)
    {
        test_branch(summary, 0x10, "BPL", components::Cpu6502Flags::N, false);
    }
    
    // void test_BRK(test::TestSummary *summary)
    // {
    //     summary->group_name = "BRK";
    //     summary->total = 1;
    //     run_test(summary, []() {
    //         return test_range_8([](uint8_t s) {
    //             uint16_t pc_irq = rand() & 0x10000;
    //             uint16_t pc = rand() & 0xFFFA;
    //             uint8_t p = rand() % 0x100;
    //             std::unordered_map<uint16_t, uint8_t> mem = {
    //                 {pc, 0x00},
    //                 {0xFFFE, pc_irq & 0xFF},
    //                 {0xFFFF, pc_irq >> 8},
    //             };
    //             return test_inst(
    //                 "BRK", &mem, 7,
    //                 [&](components::Cpu6502 *cpu) {
    //                     return 
    //                         mem[0x100 + s] == ((pc + 2) >> 8) && // hi
    //                         mem[0x100 + s - 1] == ((pc + 2) & 0xFF) && // lo
    //                         mem[0x100 + s - 2] == (p & components::Cpu6502Flags::B) &&
    //                         cpu->get_s() == (s - 3) &&
    //                         cpu->get_p() == (p & components::Cpu6502Flags::I) &&
    //                         cpu->get_pc() == pc_irq;
    //                 },
    //                 [&](char* buf, components::Cpu6502 *cpu) {
    //                     sprintf(buf, "Start | PC: 0x%04x S: 0x%02x P: 0x%02x (IRQ: 0x%04x)\n"
    //                                  "Want  | PC: 0x%04x S: 0x%02x P: 0x%02x [S]: 0x%02x [S-1]: 0x%02x [S-2]: 0x%02x\n"
    //                                  "Have  | PC: 0x%04x S: 0x%02x P: 0x%02x",
    //                                  pc, s, p, pc_irq,
    //                                  pc_irq, (s - 3) & 0xFF, p & components::Cpu6502Flags::I, pc >> 8, pc & 0xFF, p,
    //                                  cpu->get_pc(), cpu->get_s(), cpu->get_p());
    //                 },
    //                 [&](components::Cpu6502 *cpu) {
    //                     cpu->test_set_s(s);
    //                     cpu->test_set_p(p);
    //                 });
    //         }, 0x03, 0xFF);
    //     });
    // }

    void test_BVC(test::TestSummary *summary)
    {
        test_branch(summary, 0x50, "BVC", components::Cpu6502Flags::V, false);
    }
    
    void test_BVS(test::TestSummary *summary)
    {
        test_branch(summary, 0x70, "BVS", components::Cpu6502Flags::V, true);
    }
    
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

    void test_LDA(test::TestSummary *summary)
    {
        summary->group_name = "LDA";
        summary->total = 8;

        auto is_passed = [](components::Cpu6502 *cpu, uint8_t value) {
            return cpu->get_a() == value
                && check_flag(cpu, components::Cpu6502Flags::N, (value & 0x80) == 0x80)
                && check_flag(cpu, components::Cpu6502Flags::Z, value == 0);
        };

        auto debug = [](char* buf, components::Cpu6502 *cpu, uint8_t value) {
            sprintf(buf, "#0x%02x =? 0x%02x | N:%d Z:%d",
                   value, cpu->get_a(),
                   cpu->get_p() & components::Cpu6502Flags::N,
                   cpu->get_p() & components::Cpu6502Flags::Z);
        };

        run_test(summary, [&]() {
            return test_imm(0xA9, "LDA imm", is_passed, debug);
        });
        run_test(summary, [&]() {
            return test_abs(0xAD, "LDA abs", 4, is_passed, debug);
        });
        run_test(summary, [&]() {
            return test_absX(0xBD, "LDA abs,X", 4, 5, is_passed, debug);
        });
        run_test(summary, [&]() {
            return test_absY(0xB9, "LDA abs,Y", 4, 5, is_passed, debug);
        });
        run_test(summary, [&]() {
            return test_zpg(0xA5, "LDA zpg", 3, is_passed, debug);
        });
        run_test(summary, [&]() {
            return test_zpgX(0xB5, "LDA zpg,X", 4, is_passed, debug);
        });
        run_test(summary, [&]() {
            return test_Xind(0xA1, "LDA X,ind", 6, is_passed, debug);
        });
        run_test(summary, [&]() {
            return test_indY(0xB1, "LDA ind,Y", 5, 6, is_passed, debug);
        });
    }

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