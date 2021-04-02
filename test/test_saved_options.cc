#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <tinyopt/tinyopt.h>

using svector = std::vector<std::string>;

TEST(saved_options, add) {
    to::saved_options so1;
    for (auto x: {"tasty", "fish", "cake"}) so1.add(x);
    EXPECT_EQ((svector{"tasty", "fish", "cake"}), svector(so1.begin(), so1.end()));

    to::saved_options so2;
    so2.add("penguin");
    so2.add("puffin");
    so2 += so1;

    EXPECT_EQ((svector{"penguin", "puffin", "tasty", "fish", "cake"}), svector(so2.begin(), so2.end()));
}

TEST(saved_options, arglist) {
    to::saved_options so;
    for (auto x: {"tasty", "fish", "cake"}) so.add(x);

    auto A = so.as_arglist();
    EXPECT_EQ(3, A.argc);
    EXPECT_EQ("tasty", std::string(A.argv[0]));
    EXPECT_EQ("fish", std::string(A.argv[1]));
    EXPECT_EQ("cake", std::string(A.argv[2]));
    EXPECT_EQ(nullptr, A.argv[3]);
}

TEST(saved_options, serialize) {
    to::saved_options so;

    so.add("a b  c\t");
    so.add("\n");
    so.add("xyz");
    so.add("x'y'z");
    so.add("");

    std::stringstream s;
    s << so;
    EXPECT_EQ("'a b  c\t' '\n' xyz 'x'\\''y'\\''z' ''", s.str());
}

TEST(saved_options, deserialize) {
    std::stringstream s("a '' '\t' b' c 'd");
    to::saved_options so;
    s >> so;

    EXPECT_EQ((svector{"a", "", "\t", "b c d"}), svector(so.begin(), so.end()));
}
