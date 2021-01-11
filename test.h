#ifndef TEST_H
#define TEST_H

#include <string>

namespace test {

    struct TestResult {
        bool is_passed;
        bool is_partial_fail;

        std::string test_name;
        std::string message;
    };
}

#endif