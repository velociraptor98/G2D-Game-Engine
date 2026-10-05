#ifndef TESTFRAMEWORK_H
#define TESTFRAMEWORK_H
#include <iostream>
#include <string>
#include <vector>

namespace test
{
    struct TestCase
    {
        const char *name;
        void (*run)();
    };

    inline std::vector<TestCase> &AllTests()
    {
        static std::vector<TestCase> tests;
        return tests;
    }

    inline int &FailureCount()
    {
        static int failures = 0;
        return failures;
    }

    struct Registrar
    {
        Registrar(const char *name, void (*run)()) { AllTests().push_back({name, run}); }
    };

    inline bool Check(bool passed, const char *expression, const char *file, int line)
    {
        if (!passed)
        {
            ++FailureCount();
            std::clog << "  " << file << ":" << line << ": CHECK(" << expression << ") failed" << std::endl;
        }
        return passed;
    }

    template <typename A, typename B>
    bool CheckEqual(const A &actual, const B &expected, const char *actualText, const char *expectedText,
                    const char *file, int line)
    {
        if (actual == expected)
        {
            return true;
        }
        ++FailureCount();
        std::clog << "  " << file << ":" << line << ": CHECK_EQ(" << actualText << ", " << expectedText
                  << ") failed: got " << actual << ", expected " << expected << std::endl;
        return false;
    }
}

#define TEST_CONCAT_INNER(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_INNER(a, b)
#define TEST(name)                                                              \
    static void name();                                                         \
    static test::Registrar TEST_CONCAT(name, _registrar)(#name, name);          \
    static void name()

#define CHECK(condition) test::Check(static_cast<bool>(condition), #condition, __FILE__, __LINE__)
#define CHECK_EQ(actual, expected) test::CheckEqual((actual), (expected), #actual, #expected, __FILE__, __LINE__)

// Stops the current test when a precondition fails, so later lines don't
// dereference something that isn't there.
#define REQUIRE(condition)       \
    do                           \
    {                            \
        if (!CHECK(condition))   \
        {                        \
            return;              \
        }                        \
    } while (0)
#endif
