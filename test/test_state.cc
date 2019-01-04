#include <cstdlib>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

#include <tinyopt/miniopt.h>

TEST(state, shift) {
    char argvec[] = "zero\0one\0two\0three\0four\0five\0six\0\0";

#if 0
    int c = 7;
    to::state s(c, v);

    char **v0 = v;
    s.shift();
    EXPECT_EQ(6, c);
    EXPECT_EQ(0, v[c]);
    EXPECT_EQ(v0[1], v[0]);

    s.skip();
    EXPECT_EQ(6, c);
    EXPECT_EQ(0, v[c]);
    EXPECT_EQ(v0[1], v[0]);
    EXPECT_EQ(v0[2], s.argv[0]);

    s.shift(2);
    EXPECT_EQ(4, c);
    EXPECT_EQ(0, v[c]);
    EXPECT_EQ(v0[3], v[0]);
    EXPECT_EQ(v0[4], s.argv[0]);
#endif
}
