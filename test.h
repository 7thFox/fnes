#ifndef TEST_H
#define TEST_H

#include <string>
#include <iostream>
#include <iomanip>

namespace test {

    struct TestResult {
        bool is_passed;
        bool is_partial_fail;

        std::string test_name;
        std::string message;
    };

    struct TestSummary {
        std::string group_name;

        double elapsed_time;
        int total;
        int failed;
        int passed;
        int partial_pass;
        
        std::ostream* out;
        bool results_only;

        TestSummary(std::ostream *out, bool results_only)
        {
            this->passed = 0;
            this->partial_pass = 0;
            this->failed = 0;
            this->elapsed_time = 0;
            this->out = out;
            this->results_only = results_only;
        }

        void write_summary(std::ostream &out)
        {
            out << "--------------Test Summary--------------" << std::endl
                << this->group_name << " Tests:" << std::endl
                << " Total Time: " << std::fixed << std::setprecision(0) << this->elapsed_time * 1000 << " ms" << std::endl
                << " Success: " << this->passed << std::endl
                << " Failed: " << this->failed << std::endl
                << " Partial Pass: " << this->partial_pass << std::endl
                << "----------------------------------------" << std::endl
                << std::endl;
        }
    };
}

#endif