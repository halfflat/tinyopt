#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tinyopt/miniopt.h>

#include "mockargs.h"

using namespace std::literals;

TEST(state, shift) {
    const char* argstr = "zero\0one\0two\0three\0four\0five\0six\0";
    mockargs M(argstr);

    std::vector<char*> v0 = M.args;

    to::state s(M.argc, M.argv);

    s.shift();
    EXPECT_EQ(6, M.argc);
    EXPECT_EQ(0, M.argv[M.argc]);
    EXPECT_EQ(v0[1], M.argv[0]);

    s.skip();
    EXPECT_EQ(6, M.argc);
    EXPECT_EQ(0, M.argv[M.argc]);
    EXPECT_EQ(M.argv[1], s.argv[0]);
    EXPECT_EQ(v0[1], M.argv[0]);
    EXPECT_EQ(v0[2], s.argv[0]);

    s.shift(2);
    EXPECT_EQ(4, M.argc);
    EXPECT_EQ(0, M.argv[M.argc]);
    EXPECT_EQ(M.argv[1], s.argv[0]);
    EXPECT_EQ(v0[4], M.argv[1]);
    EXPECT_EQ(v0[4], s.argv[0]);
}

TEST(state, match_long) {
    to::key k("key", to::key::longfmt);

    {
        mockargs M("key\0value\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_option(k);
        EXPECT_EQ("value"s, arg);
        EXPECT_EQ("rest"s, M.argv[0]);
    }

    {
        mockargs M("key\0value\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_flag(k);
        EXPECT_EQ(true, arg);
        EXPECT_EQ("value"s, M.argv[0]);
    }

    {
        mockargs M("key=value\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_flag(k);
        EXPECT_EQ(false, arg);
        EXPECT_EQ("key=value"s, M.argv[0]);
    }

    {
        mockargs M("key=value\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_option(k);
        EXPECT_EQ("value"s, arg);
        EXPECT_EQ("rest"s, M.argv[0]);
    }

    {
        mockargs M("keyvalue\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_option(k);
        EXPECT_EQ(nullptr, arg);
        EXPECT_EQ("keyvalue"s, M.argv[0]);
    }
}

TEST(state, match_short) {
    to::key k("key", to::key::shortfmt);

    {
        mockargs M("key\0value\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_option(k);
        EXPECT_EQ("value"s, arg);
        EXPECT_EQ("rest"s, M.argv[0]);
    }

    {
        mockargs M("key\0value\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_flag(k);
        EXPECT_EQ(true, arg);
        EXPECT_EQ("value"s, M.argv[0]);
    }

    {
        mockargs M("key=value\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_option(k);
        EXPECT_EQ(nullptr, arg);
        EXPECT_EQ("key=value"s, M.argv[0]);
    }

    {
        mockargs M("key=value\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_flag(k);
        EXPECT_EQ(false, arg);
        EXPECT_EQ("key=value"s, M.argv[0]);
    }

    {
        mockargs M("keyvalue\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_option(k);
        EXPECT_EQ(nullptr, arg);
        EXPECT_EQ("keyvalue"s, M.argv[0]);
    }
}

TEST(state, match_compact) {
    to::key k("key", to::key::compact);

    {
        mockargs M("key\0value\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_option(k);
        EXPECT_EQ("value"s, arg);
        EXPECT_EQ("rest"s, M.argv[0]);
    }

    {
        mockargs M("key\0value\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_flag(k);
        EXPECT_EQ(true, arg);
        EXPECT_EQ("value"s, M.argv[0]);
    }

    {
        mockargs M("key=value\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_option(k);
        EXPECT_EQ("=value"s, arg);
        EXPECT_EQ("rest"s, M.argv[0]);
    }

    {
        mockargs M("key=value\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_flag(k);
        EXPECT_EQ(true, arg);
        EXPECT_EQ("key=value"s, M.argv[0]);
    }

    {
        mockargs M("keyvalue\0rest\0");
        to::state s(M.argc, M.argv);

        auto arg = s.match_option(k);
        EXPECT_EQ("value"s, arg);
        EXPECT_EQ("rest"s, M.argv[0]);
    }
}

TEST(state, match_multi_compact) {
    to::key k1("key/one", to::key::compact);
    to::key k2("key/two", to::key::compact);
    to::key k3("key/three", to::key::compact);
    to::key k4("key/four", to::key::compact);

    {
        mockargs M("key/one/three/two\0key/four\0rest\0");
        to::state s(M.argc, M.argv);

        EXPECT_TRUE(s.match_flag(k1));
        EXPECT_FALSE(s.match_flag(k2));
        EXPECT_TRUE(s.match_flag(k3));
        EXPECT_TRUE(s.match_flag(k2));
        EXPECT_TRUE(s.match_flag(k4));
        EXPECT_EQ("rest"s, M.argv[0]);
    }
}
