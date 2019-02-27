#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tinyopt/tinyopt.h>

#include "mockargs.h"

using namespace std::literals;

TEST(tinyopt, parse) {
    {
        mockargs M("-s\0end\0");
        char** arg = M.argv;

        auto r = to::parse(arg, 's', "long");
        ASSERT_TRUE(r);
        ASSERT_EQ(M.argv+1, arg);
    }

    {
        mockargs M("--s\0end\0");
        char** arg = M.argv;
        auto r = to::parse(arg, 's', "long");

        ASSERT_FALSE(r);
        ASSERT_EQ(M.argv, arg);
    }

    {
        mockargs M("--long\0end\0");
        char** arg = M.argv;
        auto r = to::parse(arg, 's', "long");

        ASSERT_TRUE(r);
        ASSERT_EQ(M.argv+1, arg);
    }

    {
        mockargs M("-long\0end\0");
        char** arg = M.argv;
        auto r = to::parse(arg, 's', "long");

        ASSERT_FALSE(r);
        ASSERT_EQ(M.argv, arg);
    }

    {
        mockargs M("--xyz\0end\0");
        char** arg = M.argv;
        auto r = to::parse(arg, 's', "long");

        ASSERT_FALSE(r);
        ASSERT_EQ(M.argv, arg);
    }
}

TEST(tinyopt, parse_value) {
    {
        mockargs M("-s\0003\0end\0");
        char** arg = M.argv;

        auto r = to::parse<int>(arg, 's', "long");
        ASSERT_TRUE(r);
        ASSERT_EQ(3, *r);
        ASSERT_EQ(M.argv+2, arg);
    }

    {
        mockargs M("-s\0fish\0");
        char** arg = M.argv;

        EXPECT_THROW(to::parse<int>(arg, 's', "long"), to::option_parse_error);
    }

    {
        mockargs M("--long\0-123\0end\0");
        char** arg = M.argv;

        auto r = to::parse<int>(arg, 's', "long");
        ASSERT_TRUE(r);
        ASSERT_EQ(-123, *r);
        ASSERT_EQ(M.argv+2, arg);
    }

    {
        mockargs M("--long\0");
        char** arg = M.argv;

        EXPECT_THROW(to::parse<int>(arg, 's', "long"), to::missing_argument);
    }

    {
        mockargs M("--xyz\0end\0");
        char** arg = M.argv;

        auto r = to::parse<int>(arg, 's', "long");
        ASSERT_FALSE(r);
        ASSERT_EQ(M.argv, arg);
    }
}

TEST(tinyopt, custom_parser) {
    auto custom = [](const char* s) -> to::maybe<bool> {
        if (!std::strcmp("no", s)) return false;
        if (!std::strcmp("yes", s)) return true;
        return to::nothing;
    };

    {
        mockargs M("-s\0yes\0");
        char** arg = M.argv;

        auto r = to::parse<bool>(arg, 's', "long", custom);
        ASSERT_TRUE(r);
        ASSERT_EQ(true, *r);
        ASSERT_EQ(M.argv+2, arg);
    }

    {
        mockargs M("-s\0no\0");
        char** arg = M.argv;

        auto r = to::parse<bool>(arg, 's', "long", custom);
        ASSERT_TRUE(r);
        ASSERT_EQ(false, *r);
        ASSERT_EQ(M.argv+2, arg);
    }

    {
        mockargs M("-s\0what?\0");
        char** arg = M.argv;

        EXPECT_THROW(to::parse<bool>(arg, 's', "long", custom), to::option_parse_error);
    }
}
