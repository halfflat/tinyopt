#include <gtest/gtest.h>

#include <tinyopt/miniopt.h>

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
