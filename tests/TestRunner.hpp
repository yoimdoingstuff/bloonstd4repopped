#pragma once

#include <iostream>
#include <stdexcept>
#include <string>

namespace btd4::test {

using TestFunction = void (*)();

struct TestCase {
    const char* name;
    TestFunction func;
};

class TestRunner {
public:
    static TestRunner& instance() {
        static TestRunner s_instance;
        return s_instance;
    }

    void registerTest(const char* name, TestFunction func) {
        if (m_testCount >= kMaxTests) {
            std::cerr << "Too many tests registered; increase TestRunner::kMaxTests." << std::endl;
            std::terminate();
        }
        m_tests[m_testCount++] = {name, func};
    }

    int run() {
        int passed = 0;
        int failed = 0;

        std::cout << "========================================" << std::endl;
        std::cout << " Running " << m_testCount << " test suites..." << std::endl;
        std::cout << "========================================" << std::endl;

        for (size_t index = 0; index < m_testCount; ++index) {
            const TestCase& test = m_tests[index];
            std::cout << "[ RUN      ] " << test.name << std::endl;
            try {
                test.func();
                std::cout << "[       OK ] " << test.name << std::endl;
                ++passed;
            } catch (const std::exception& e) {
                std::cerr << "[  FAILED  ] " << test.name << ": " << e.what() << std::endl;
                ++failed;
            } catch (...) {
                std::cerr << "[  FAILED  ] " << test.name << ": Unknown exception" << std::endl;
                ++failed;
            }
        }

        std::cout << "========================================" << std::endl;
        std::cout << " Tests finished: " << passed << " passed, " << failed << " failed." << std::endl;
        std::cout << "========================================" << std::endl;

        return (failed == 0) ? 0 : 1;
    }

private:
    static constexpr size_t kMaxTests = 256;
    TestCase m_tests[kMaxTests]{};
    size_t m_testCount{0};
};

struct TestRegistrar {
    TestRegistrar(const char* name, TestFunction func) {
        TestRunner::instance().registerTest(name, func);
    }
};

#define TEST_CASE(name) \
    static void _test_##name(); \
    static ::btd4::test::TestRegistrar _reg_##name(#name, _test_##name); \
    static void _test_##name()

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            throw std::runtime_error(std::string("Assertion failed: ") + #cond + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (false)

#define TEST_ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            throw std::runtime_error(std::string("Assertion failed: ") + #a + " == " + #b + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (false)

} // namespace btd4::test
