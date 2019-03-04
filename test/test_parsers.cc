#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tinyopt/tinyopt.h>

using namespace std::literals;

TEST(tinyopt, default_parser) {
    auto p1 = to::default_parser<const char*>()("  test string ");
    ASSERT_TRUE(p1);
    EXPECT_EQ(0, std::strcmp("  test string ", *p1));

    auto p2 = to::default_parser<std::string>()("  test string ");
    ASSERT_TRUE(p1);
    EXPECT_EQ("  test string "s, *p1);

    auto p3 = to::default_parser<std::string>()("");
    ASSERT_TRUE(p3);
    EXPECT_TRUE(p3->empty());

    auto p4 = to::default_parser<int>()("");
    EXPECT_FALSE(p4);

    auto p5 = to::default_parser<int>()("abc");
    EXPECT_FALSE(p5);

    auto p6 = to::default_parser<int>()("-123x");
    EXPECT_FALSE(p6);

    auto p7 = to::default_parser<int>()("-123");
    ASSERT_TRUE(p7);
    EXPECT_EQ(-123, *p7);

    auto p8 = to::default_parser<int>()(" -123   ");
    ASSERT_TRUE(p8);
    EXPECT_EQ(-123, *p8);
}

TEST(tinyopt, keywords) {
    std::pair<const char*, int> kw[] = {
        { "one", 1 }, { "two", 2}
    };

    auto parser = to::keywords(kw);

    auto p1 = parser("one");
    ASSERT_TRUE(p1);
    EXPECT_EQ(1, *p1);

    auto p2 = parser("two");
    ASSERT_TRUE(p2);
    EXPECT_EQ(2, *p2);

    EXPECT_FALSE(parser("three"));
    EXPECT_FALSE(parser(""));
    EXPECT_FALSE(parser(" one"));
    EXPECT_FALSE(parser("one "));
    EXPECT_FALSE(parser("on"));
}

TEST(tinyopt, delimited) {
    using ivector = std::vector<int>;
    auto parser = to::delimited<int>('/');

    auto p1 = parser("1/2/3");
    ASSERT_TRUE(p1);
    EXPECT_EQ((ivector{1, 2, 3}),*p1);

    auto p2 = parser("");
    ASSERT_TRUE(p2);
    EXPECT_EQ((ivector{}),*p2);

    EXPECT_FALSE(parser("/2"));
    EXPECT_FALSE(parser("1/"));
    EXPECT_FALSE(parser("1a/2"));
}

TEST(tinyopt, delimited_empty) {
    auto parser = to::delimited<std::string>('/');

    auto p1 = parser("");
    ASSERT_TRUE(p1);
    EXPECT_EQ(0u, p1->size());

    auto p2 = parser("/");
    ASSERT_TRUE(p2);
    EXPECT_EQ(2u, p2->size());

    auto p3 = parser("//");
    ASSERT_TRUE(p3);
    EXPECT_EQ(3u, p3->size());

    auto p4 = parser("a/");
    ASSERT_TRUE(p4);
    EXPECT_EQ(2u, p4->size());
    EXPECT_EQ("a"s, p4->at(0));
    EXPECT_EQ(""s, p4->at(1));

    auto p5 = parser("/a");
    ASSERT_TRUE(p5);
    EXPECT_EQ(2u, p5->size());
    EXPECT_EQ(""s, p5->at(0));
    EXPECT_EQ("a"s, p5->at(1));
}

TEST(tinyopt, delimited_custom) {
    using ivector = std::vector<int>;

    std::pair<const char*, int> kw[] = {
        { "one", 1 }, { "two", 2}
    };

    auto kw_parser = to::keywords(kw);
    auto delim_parser = to::delimited(',', kw_parser);

    auto p1 = delim_parser("one,one,two");
    ASSERT_TRUE(p1);
    EXPECT_EQ((ivector{1, 1, 2}),*p1);

    auto p2 = delim_parser("");
    ASSERT_TRUE(p2);
    EXPECT_EQ((ivector{}),*p2);

    EXPECT_FALSE(delim_parser("one, one,two"));
    EXPECT_FALSE(delim_parser("one,three"));
}
