#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <tinyopt/tinyopt.h>
#include "mockargs.h"

using namespace std::literals;

TEST(state, shift) {
    const char* argstr = "zero\0one\0two\0three\0four\0five\0six\0";
    mockargs M(argstr);

    std::vector<char*> v0 = M.args;
    to::state s(M.argc, M.argv);

    s.shift(0);
    EXPECT_EQ(7, M.argc);
    EXPECT_EQ(0, M.argv[M.argc]);
    EXPECT_EQ(v0[0], M.argv[0]);

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

TEST(state, consume) {
    const char* argstr = "zero\0one\0two\0three\0four\0five\0six\0";
    mockargs M(argstr);

    std::vector<char*> v0 = M.args;
    to::state s(M.argc, M.argv);

    typedef to::state::match_result MR;


    s.consume(MR{nullptr, 0, 0});
    EXPECT_EQ(7, M.argc);
    EXPECT_EQ(0, M.argv[M.argc]);
    EXPECT_EQ(v0[0], M.argv[0]+s.optoff);

    s.consume(MR{nullptr, 0, 2});
    EXPECT_EQ(7, M.argc);
    EXPECT_EQ(0, M.argv[M.argc]);
    EXPECT_EQ(v0[0]+2, M.argv[0]+s.optoff);

    s.consume(MR{nullptr, 2, 0});
    EXPECT_EQ(5, M.argc);
    EXPECT_EQ(0, M.argv[M.argc]);
    EXPECT_EQ(v0[2], M.argv[0]+s.optoff);

    s.consume(MR{nullptr, 0, 2});
    EXPECT_EQ(5, M.argc);
    EXPECT_EQ(0, M.argv[M.argc]);
    EXPECT_EQ(v0[2]+2, M.argv[0]+s.optoff);

    s.consume(MR{nullptr, 1, 1});
    EXPECT_EQ(4, M.argc);
    EXPECT_EQ(0, M.argv[M.argc]);
    EXPECT_EQ(v0[3]+1, M.argv[0]+s.optoff);
}

TEST(state, match_long) {
    to::key k("key", to::key::longfmt);

    {
        mockargs M("key\0value\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_option(k);
        ASSERT_TRUE(mmr);
        EXPECT_EQ("value"s, mmr->argument);
        EXPECT_EQ(2u, mmr->shift);
        EXPECT_EQ(0u, mmr->offset);

        s.consume(*mmr);
        EXPECT_EQ("rest"s, M.argv[0]);
    }

    {
        mockargs M("key\0value\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_flag(k);
        ASSERT_TRUE(mmr);
        EXPECT_EQ(nullptr, mmr->argument);
        EXPECT_EQ(1u, mmr->shift);
        EXPECT_EQ(0u, mmr->offset);

        s.consume(*mmr);
        EXPECT_EQ("value"s, M.argv[0]);
    }

    {
        mockargs M("key=value\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_flag(k);
        ASSERT_FALSE(mmr);
    }

    {
        mockargs M("key=value\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_option(k);
        ASSERT_TRUE(mmr);
        EXPECT_EQ("value"s, mmr->argument);
        EXPECT_EQ(1u, mmr->shift);
        EXPECT_EQ(0u, mmr->offset);

        s.consume(*mmr);
        EXPECT_EQ("rest"s, M.argv[0]);
    }

    {
        mockargs M("keyvalue\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_option(k);
        ASSERT_FALSE(mmr);
    }
}

TEST(state, match_short) {
    to::key k("key", to::key::shortfmt);

    {
        mockargs M("key\0value\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_option(k);
        ASSERT_TRUE(mmr);
        EXPECT_EQ("value"s, mmr->argument);
        EXPECT_EQ(2u, mmr->shift);
        EXPECT_EQ(0u, mmr->offset);

        s.consume(*mmr);
        EXPECT_EQ("rest"s, M.argv[0]);
    }

    {
        mockargs M("key\0value\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_flag(k);
        ASSERT_TRUE(mmr);
        EXPECT_EQ(nullptr, mmr->argument);
        EXPECT_EQ(1u, mmr->shift);
        EXPECT_EQ(0u, mmr->offset);

        s.consume(*mmr);
        EXPECT_EQ("value"s, M.argv[0]);
    }

    {
        mockargs M("key=value\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_option(k);
        ASSERT_FALSE(mmr);
    }

    {
        mockargs M("key=value\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_option(k);
        ASSERT_FALSE(mmr);
    }

    {
        mockargs M("keyvalue\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_option(k);
        ASSERT_FALSE(mmr);
    }
}

TEST(state, match_compact) {
    to::key k("key", to::key::compact);

    {
        mockargs M("key\0value\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_option(k);
        ASSERT_TRUE(mmr);
        EXPECT_EQ("value"s, mmr->argument);
        EXPECT_EQ(2, mmr->shift);
        EXPECT_EQ(0, mmr->offset);

        s.consume(*mmr);
        EXPECT_EQ("rest"s, M.argv[0]);
    }

    {
        mockargs M("key\0value\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_flag(k);
        ASSERT_TRUE(mmr);
        EXPECT_EQ(nullptr, mmr->argument);
        EXPECT_EQ(1, mmr->shift);
        EXPECT_EQ(0, mmr->offset);

        s.consume(*mmr);
        EXPECT_EQ("value"s, M.argv[0]);
    }

    {
        mockargs M("key=value\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_option(k);
        ASSERT_TRUE(mmr);
        EXPECT_EQ("=value"s, mmr->argument);
        EXPECT_EQ(1, mmr->shift);
        EXPECT_EQ(0, mmr->offset);

        s.consume(*mmr);
        EXPECT_EQ("rest"s, M.argv[0]);
    }

    {
        mockargs M("key=value\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_flag(k);
        ASSERT_TRUE(mmr);
        EXPECT_EQ(nullptr, mmr->argument);
        EXPECT_EQ(0, mmr->shift);
        EXPECT_EQ(3, mmr->offset);

        s.consume(*mmr);
        EXPECT_EQ("key=value"s, M.argv[0]);
        EXPECT_EQ("=value"s, M.argv[0]+s.optoff);
    }

    {
        mockargs M("keyvalue\0rest\0");
        to::state s(M.argc, M.argv);

        auto mmr = s.match_option(k);
        ASSERT_TRUE(mmr);
        EXPECT_EQ("value"s, mmr->argument);
        EXPECT_EQ(1, mmr->shift);
        EXPECT_EQ(0, mmr->offset);

        s.consume(*mmr);
        EXPECT_EQ("rest"s, M.argv[0]+s.optoff);
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

        to::maybe<to::state::match_result> mmr;

        ASSERT_TRUE(mmr = s.match_flag(k1));
        s.consume(*mmr);

        ASSERT_FALSE(mmr = s.match_flag(k2));

        ASSERT_TRUE(mmr = s.match_flag(k3));
        s.consume(*mmr);

        ASSERT_TRUE(mmr = s.match_flag(k2));
        s.consume(*mmr);

        ASSERT_TRUE(mmr = s.match_flag(k4));
        s.consume(*mmr);

        EXPECT_EQ("rest"s, M.argv[0]);
    }
}
