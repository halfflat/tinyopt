#include <cstdlib>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

#include <tinyopt/miniopt.h>

struct mockargs {
    explicit mockargs(const char* argstr) {
        set(argstr);
    }

    void set(const char* argstr) {
        char* p = const_cast<char*>(argstr);
        args.push_back(p);
        for (; *p; ) {
            while (*p++) ;
            args.push_back(p);
        }
        args.back() = 0;
        argv = args.data();
        argc = args.size();
    }

    int argc;
    char** argv;
    std::vector<char*> args;
};

TEST(state, shift) {
    const char* argstr = "zero\0one\0two\0three\0four\0five\0six\0\0";
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
    EXPECT_EQ(v0[1], M.argv[0]);
    EXPECT_EQ(v0[2], s.argv[0]);

    s.shift(2);
    EXPECT_EQ(4, M.argc);
    EXPECT_EQ(0, M.argv[M.argc]);
    EXPECT_EQ(v0[3], M.argv[0]);
    EXPECT_EQ(v0[4], s.argv[0]);
}
