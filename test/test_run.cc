#include <cstdlib>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <tinyopt/tinyopt.h>
#include "mockargs.h"

using namespace std::literals;
using svector = std::vector<std::string>;

TEST(run, flags) {
    bool a = false, b = false;
    to::option opts[] = {
        { to::set(a), "-a", to::flag },
        { to::set(b), "-b", to::flag }
    };

    mockargs M1("-a\0-b\0");
    auto r1 = to::run(opts, M1.argc, M1.argv);
    EXPECT_TRUE(a);
    EXPECT_TRUE(b);
    EXPECT_EQ(0, M1.argc);
    EXPECT_EQ(nullptr, *M1.argv);
    ASSERT_TRUE(r1);
    EXPECT_EQ((svector{"-a", "-b"}), svector(r1->begin(), r1->end()));

    a = b = false;
    mockargs M2("-c\0");
    auto r2 = to::run(opts, M2.argc, M2.argv);
    EXPECT_FALSE(a);
    EXPECT_FALSE(b);
    EXPECT_EQ(1, M2.argc);
    EXPECT_EQ("-c"s, *M2.argv);
    ASSERT_TRUE(r2);
    EXPECT_TRUE(r2->empty());
}

TEST(run, exit) {
    int a = 0;
    int b = 0;

    to::option opts[] = {
        { a, "-a", to::exit },
        { b, "-b" }
    };

    mockargs M("-a\09\0-b\08\0");
    auto r = to::run(opts, M.argc, M.argv);
    EXPECT_FALSE(r);
    EXPECT_EQ(9, a);
    EXPECT_EQ(0, b);
    EXPECT_EQ(2, M.argc);
}

TEST(run, ephemeral) {
    int a = 0;
    int b = 0;

    to::option opts[] = {
        { a, "-a", to::ephemeral },
        { b, "-b" }
    };

    mockargs M("-a\09\0-b\08\0");
    auto r = to::run(opts, M.argc, M.argv);
    EXPECT_EQ(9, a);
    EXPECT_EQ(8, b);
    EXPECT_EQ(0, M.argc);
    ASSERT_TRUE(r);
    EXPECT_EQ((svector{"-b", "8"}), svector(r->begin(), r->end()));
}

TEST(run, end_of_options) {
    std::string a, b;
    to::option opts[] = {
        { a, "-a" },
        { b, "-b" }
    };

    mockargs M1("-a\0foo\0--\0-b\0bar\0");
    auto r1 = to::run(opts, M1.argc, M1.argv);
    EXPECT_EQ("foo"s, a);
    EXPECT_EQ(""s, b);
    EXPECT_EQ(2, M1.argc);
    EXPECT_TRUE(r1);

    // '--' can still be an argument to an option, however;
    a = b = "";
    mockargs M2("-a\0--\0foo\0-b\0bar\0");
    auto r2 = to::run(opts, M2.argc, M2.argv);
    EXPECT_EQ("--"s, a);
    EXPECT_EQ("bar"s, b);
    EXPECT_EQ(1, M2.argc); // 1 unmatched argument 'foo'
    EXPECT_EQ("foo"s, *M2.argv);
    EXPECT_TRUE(r2);
}

TEST(run, single) {
    int c = 0, d = 0;
    to::option opts[] = {
        { to::increment(c), "-c", to::flag, to::single },
        { to::increment(d), "-c", to::flag, to::single }
    };

    mockargs M("-c\0-c\0-c\0");
    auto r = to::run(opts, M.argc, M.argv);
    EXPECT_EQ(1, c);
    EXPECT_EQ(1, d);
    EXPECT_EQ(1, M.argc);
    EXPECT_TRUE(r);
}

TEST(run, mandatory) {
    bool m = false, a = false;
    to::option opts[] = {
        { to::set(m), "-m", to::flag, to::mandatory },
        { to::set(a), "-a", to::flag }
    };

    mockargs M1("-a\0-m\0");
    auto r1 = to::run(opts, M1.argc, M1.argv);
    EXPECT_TRUE(r1);
    EXPECT_TRUE(a);
    EXPECT_TRUE(m);

    mockargs M2("-a\0");
    EXPECT_THROW(to::run(opts, M2.argc, M2.argv), to::missing_mandatory_option);
}

TEST(run, restore) {
    int c = 0;
    to::option opts[] = {
        { to::increment(c), "-c", to::flag }
    };

    to::saved_options restore;
    restore.add("-c");
    restore.add("-c");

    auto r1 = to::run(opts, restore);
    EXPECT_TRUE(r1);
    EXPECT_EQ(2, c);

    c = 0;
    mockargs M2("-c\0-c\0-c\0");
    auto r2 = to::run(opts, M2.argc, M2.argv, restore);
    EXPECT_TRUE(r2);
    EXPECT_EQ(5, c);
}

TEST(run, restore_single) {
    int c = 0, d = 0;
    to::option opts[] = {
        { to::increment(c), "-c", to::flag, to::single },
        { to::increment(d), "-c", to::flag, to::single }
    };

    to::saved_options restore;
    restore.add("-c");

    mockargs M("-c\0-c\0-c\0");
    auto r = to::run(opts, M.argc, M.argv, restore);
    EXPECT_EQ(1, c);
    EXPECT_EQ(1, d);
    EXPECT_EQ(2, M.argc);
    EXPECT_TRUE(r);
}

TEST(run, restore_mandatory) {
    bool m = false, a = false;
    to::option opts[] = {
        { to::set(m), "-m", to::flag, to::mandatory },
        { to::set(a), "-a", to::flag }
    };

    to::saved_options restore1;
    restore1.add("-m");
    mockargs M1("-a\0");

    auto r1 = to::run(opts, M1.argc, M1.argv, restore1);
    EXPECT_TRUE(r1);
    EXPECT_TRUE(a);
    EXPECT_TRUE(m);

    a = m = false;

    to::saved_options restore2;
    restore2.add("-a");
    mockargs M2("-m\0");

    auto r2 = to::run(opts, M2.argc, M2.argv, restore2);
    EXPECT_TRUE(r2);
    EXPECT_TRUE(a);
    EXPECT_TRUE(m);

    to::saved_options restore3;
    restore3.add("-a");
    mockargs M3("-a\0");

    EXPECT_THROW(to::run(opts, M3.argc, M3.argv), to::missing_mandatory_option);
}

TEST(run, keyless) {
    bool a = false;
    std::string kl_first;
    int kl_second = 0;

    to::option opts[] = {
        {kl_first, to::single},
        {kl_second, to::single},
        {to::set(a), "-a", to::flag}
    };

    mockargs M1("first\0002\0");
    auto r1 = to::run(opts, M1.argc, M1.argv);
    ASSERT_TRUE(r1);
    EXPECT_EQ((svector{"first", "2"}), svector(r1->begin(), r1->end()));
    EXPECT_EQ("first"s, kl_first);
    EXPECT_EQ(2, kl_second);

    a = false;
    kl_first = "";
    kl_second = 0;

    mockargs M2("-a\0foo\0003\0");
    auto r2 = to::run(opts, M2.argc, M2.argv);
    ASSERT_TRUE(r2);
    EXPECT_EQ((svector{"-a", "foo", "3"}), svector(r2->begin(), r2->end()));
    EXPECT_TRUE(a);
    EXPECT_EQ("foo"s, kl_first);
    EXPECT_EQ(3, kl_second);
}

TEST(run, modal) {
    int a0 = 0, a1 = 0, a2 = 0;
    to::option opts[] = {
        {to::increment(a0), "-a", to::flag, to::when(0)},
        {to::increment(a1), "-a", to::flag, to::when(1)},
        {to::increment(a2), "-a", to::flag, to::when(2)},
        {{}, "one", to::flag, to::then(1)},
        {{}, "two", to::flag, to::then(2)}
    };

    mockargs M("-a\0one\0-a\0two\0-a\0-a\0one\0-a\0-a\0");
    auto r = to::run(opts, M.argc, M.argv);
    EXPECT_TRUE(r);

    EXPECT_EQ(1, a0);
    EXPECT_EQ(3, a1);
    EXPECT_EQ(2, a2);
}
