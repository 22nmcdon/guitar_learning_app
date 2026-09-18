#pragma once

// A deliberately tiny test harness: the engine has no third-party dependencies
// and the tests do not add one. Registration happens at static-init time, so a
// test file only needs to include this header.

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace guitar::test
{

struct TestCase
{
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar
{
    Registrar (std::string name, std::function<void()> body)
    {
        registry().push_back ({ std::move (name), std::move (body) });
    }
};

struct Failure : std::exception
{
    explicit Failure (std::string m) : message (std::move (m)) {}
    const char* what() const noexcept override { return message.c_str(); }
    std::string message;
};

inline void fail (const std::string& expression, const std::string& detail,
                  const char* file, int line)
{
    std::ostringstream out;
    out << file << ":" << line << "\n      " << expression;

    if (! detail.empty())
        out << "\n      " << detail;

    throw Failure (out.str());
}

/** Runs every registered test; returns a process exit code. */
inline int runAll()
{
    auto failures = 0;

    for (const auto& test : registry())
    {
        try
        {
            test.body();
            std::cout << "  PASS  " << test.name << "\n";
        }
        catch (const Failure& failure)
        {
            ++failures;
            std::cout << "  FAIL  " << test.name << "\n      " << failure.what() << "\n";
        }
        catch (const std::exception& e)
        {
            ++failures;
            std::cout << "  FAIL  " << test.name << " (threw: " << e.what() << ")\n";
        }
    }

    std::cout << "\n" << registry().size() - static_cast<std::size_t> (failures)
              << "/" << registry().size() << " tests passed\n";

    return failures == 0 ? 0 : 1;
}

} // namespace guitar::test

#define GUITAR_CONCAT_INNER(a, b) a##b
#define GUITAR_CONCAT(a, b) GUITAR_CONCAT_INNER(a, b)

#define TEST(name)                                                                        \
    static void GUITAR_CONCAT (guitarTest, __LINE__)();                                       \
    static const ::guitar::test::Registrar GUITAR_CONCAT (guitarRegistrar, __LINE__)            \
        (name, [] { GUITAR_CONCAT (guitarTest, __LINE__)(); });                               \
    static void GUITAR_CONCAT (guitarTest, __LINE__)()

#define CHECK(condition)                                                                  \
    do {                                                                                  \
        if (! (condition))                                                                \
            ::guitar::test::fail (#condition, {}, __FILE__, __LINE__);                      \
    } while (false)

#define CHECK_EQ(actual, expected)                                                        \
    do {                                                                                  \
        const auto guitarActual = (actual);      /* by value: (actual) may be a */        \
        const auto guitarExpected = (expected);  /* reference into a temporary   */        \
        if (! (guitarActual == guitarExpected))                                               \
        {                                                                                 \
            std::ostringstream guitarDetail;                                                \
            guitarDetail << "expected: " << guitarExpected << "\n      actual:   " << guitarActual; \
            ::guitar::test::fail (#actual " == " #expected, guitarDetail.str(), __FILE__, __LINE__); \
        }                                                                                 \
    } while (false)
