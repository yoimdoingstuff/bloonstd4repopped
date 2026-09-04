#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>

namespace btd4::test {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

class TestRunner {
public:
    static TestRunner& instance() {
        static TestRunner s_instance;
        return s_instance;
    }

    void registerTest(const std::string& name, std::function<void()> func) {
        m_tests.push_back({name, std::move(func)});
    }

    int run() {
        int passed = 0;
        int failed = 0;

        std::cout << "========================================" << std::endl;
        std::cout << " Running " << m_tests.size() << " test suites..." << std::endl;
        std::cout << "========================================" << std::endl;

        for (const auto& test : m_tests) {
            std::cout << "[ RUN      ] " << test.name << std::endl;
            try {
                test.func();
                std::cout << "[       OK ] " << test.name << std::endl;
                passed++;
            } catch (const std::exception& e) {
                std::cerr << "[  FAILED  ] " << test.name << ": " << e.what() << std::endl;
                failed++;
            } catch (...) {
                std::cerr << "[  FAILED  ] " << test.name << ": Unknown exception" << std::endl;
                failed++;
            }
        }

        std::cout << "========================================" << std::endl;
        std::cout << " Tests finished: " << passed << " passed, " << failed << " failed." << std::endl;
        std::cout << "========================================" << std::endl;

        return (failed == 0) ? 0 : 1;
    }

private:
    std::vector<TestCase> m_tests;
};

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> func) {
        TestRunner::instance().registerTest(name, std::move(func));
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
