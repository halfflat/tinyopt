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
    auto voidsetter = [&y]() { ++y; };

    auto b = to::action(voidsetter);
    EXPECT_TRUE(b(nullptr));
    EXPECT_EQ(1, y);
    EXPECT_TRUE(b("wizard"));
    EXPECT_EQ(2, y);
}

TEST(sink, adaptors) {
    std::vector<double> vs;
    auto a1 = to::push_back(vs,
        [](const char* s) -> to::maybe<double> {
            if (!s) return to::nothing;
            return s[0]? 1.0: -1.0;
        });

    EXPECT_FALSE(a1(nullptr));
    EXPECT_TRUE(a1("x"));
    EXPECT_TRUE(a1(""));
    EXPECT_TRUE(a1("x"));
    EXPECT_EQ((std::vector<double>{1., -1., 1.}), vs);

    auto a2 = to::push_back(vs);
    EXPECT_FALSE(a2(nullptr));
    EXPECT_FALSE(a2("fish"));
    EXPECT_TRUE(a2("3"));
    EXPECT_TRUE(a2("-0.5e1"));
    EXPECT_EQ((std::vector<double>{1., -1., 1., 3., -5}), vs);

    int x = 3;
    auto a3 = to::set_value(x, 17);
    EXPECT_TRUE(a3(nullptr));
    EXPECT_EQ(17, x);
    x = 2;
    EXPECT_TRUE(a3("fish"));
    EXPECT_EQ(17, x);
}
