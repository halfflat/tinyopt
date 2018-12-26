#include <cstdlib>
#include <string>

#include <cxxabi.h>

#include <gtest/gtest.h>

#include <tinyopt/miniopt.h>

std::string demangle(const char* name) {
    int status = 0;
    const char* buf = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    if (buf) {
        std::string d(buf);
        free((void*)buf);
        return d;
    }
    return "";
}

TEST(sink, refctor) {
    int a;
    to::sink sa(a);

    EXPECT_TRUE(sa("312"));
    EXPECT_EQ(312, a);

    EXPECT_FALSE(sa("foo"));
    EXPECT_EQ(312, a);

    to::sink sb(a, [](const char*) { return to::just(17); });
    EXPECT_TRUE(sb("foo"));
    EXPECT_EQ(17, a);
}

TEST(sink, action_explicit) {
    char x;
    auto setter = [&x](char c) { x = c; };
    auto firstchar = [](const char* s) { return s? to::just(s[0]): to::nothing; };

    auto a = to::action(setter, firstchar);
    EXPECT_TRUE(a("fish"));
    EXPECT_EQ('f', x);

    EXPECT_FALSE(a(nullptr));
    EXPECT_EQ('f', x);
}

TEST(sink, action_implicit) {
    int n = 0;
    auto setter = [&n](int y) { n = y; };

    auto a = to::action(setter);
    EXPECT_TRUE(a("17"));
    EXPECT_EQ(17, n);

    EXPECT_FALSE(a("fishsticks"));
    EXPECT_EQ(17, n);

    int y = 0;
    auto vsetter = [&y]() { ++y; };

    auto b = to::action(vsetter);
    EXPECT_TRUE(b(nullptr));
    EXPECT_EQ(1, y);
    EXPECT_TRUE(b("wizard"));
    EXPECT_EQ(2, y);
}
