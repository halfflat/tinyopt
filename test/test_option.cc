#include <cstdlib>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tinyopt/miniopt.h>

TEST(key, ctor) {
    to::key a("--long");
    EXPECT_EQ("--long", a.label);
    EXPECT_EQ(to::key::longfmt, a.style);

    to::key b("-short");
    EXPECT_EQ("-short", b.label);
    EXPECT_EQ(to::key::shortfmt, b.style);

    to::key c("key");
    EXPECT_EQ("key", c.label);
    EXPECT_EQ(to::key::shortfmt, c.style);

    to::key d("--short", to::key::shortfmt);
    EXPECT_EQ("--short", d.label);
    EXPECT_EQ(to::key::shortfmt, d.style);

    to::key e("c", to::key::compact);
    EXPECT_EQ("c", e.label);
    EXPECT_EQ(to::key::compact, e.style);
}

TEST(key, literal) {
    using namespace to::literals;

    auto a = "-f"_short;
    auto b = "-f"_compact;
    auto c = "-f"_long;

    EXPECT_EQ("-f", a.label);
    EXPECT_EQ("-f", b.label);
    EXPECT_EQ("-f", c.label);

    EXPECT_EQ(to::key::shortfmt, a.style);
    EXPECT_EQ(to::key::compact, b.style);
    EXPECT_EQ(to::key::longfmt, c.style);
}

TEST(option, ctor) {
    using namespace to::literals;
    int a, b, c, d;
    std::string e;

    to::option opts[] = {
        {a, to::ephemeral, to::single, "-a", "--arg"},
        {b, to::mandatory, "-b", "--bob"},
        {to::increment(c), to::flag, "-c", "--cat"},
        {to::action([&d](int) {++d;}), "-d"_compact},
        {e},
    };

    EXPECT_FALSE(opts[0].is_flag);
    EXPECT_TRUE(opts[0].is_ephemeral);
    EXPECT_TRUE(opts[0].is_single);
    EXPECT_FALSE(opts[0].is_mandatory);

    EXPECT_FALSE(opts[1].is_flag);
    EXPECT_FALSE(opts[1].is_ephemeral);
    EXPECT_FALSE(opts[1].is_single);
    EXPECT_TRUE(opts[1].is_mandatory);

    EXPECT_TRUE(opts[2].is_flag);
    EXPECT_FALSE(opts[2].is_ephemeral);
    EXPECT_FALSE(opts[2].is_single);
    EXPECT_FALSE(opts[2].is_mandatory);

    EXPECT_FALSE(opts[3].is_flag);
    EXPECT_FALSE(opts[3].is_ephemeral);
    EXPECT_FALSE(opts[3].is_single);
    EXPECT_FALSE(opts[3].is_mandatory);

    using svec = std::vector<std::string>;
    auto key_labels = [](const to::option& o) {
        svec labels;
        for (auto& k: o.keys) labels.push_back(k.label);
        return labels;
    };

    EXPECT_EQ((svec{"-a", "--arg"}), key_labels(opts[0]));
    EXPECT_EQ((svec{"-d"}), key_labels(opts[3]));
    EXPECT_EQ((svec{}), key_labels(opts[4]));
}

TEST(option, run) {
    using namespace to::literals;
    int a, b, c, d;
    std::string e;

    to::option opt_a{a, to::single, "-a", "--arg"};
    ASSERT_NO_THROW(opt_a.run("-a", "3"));
    EXPECT_EQ(a, 3);
    ASSERT_THROW(opt_a.run("-a", "fish"), to::option_parse_error);

    c = 1;
    to::option opt_c{to::increment(c), to::flag, "-c", "--cat"};
    ASSERT_NO_THROW(opt_c.run("-c", nullptr));
    EXPECT_EQ(c, 2);

    d = 3;
    to::option opt_d{to::action([&d](int) {++d;}), "-d"_compact};
    ASSERT_NO_THROW(opt_d.run("-d", "7"));
    EXPECT_EQ(d, 4);
    ASSERT_THROW(opt_d.run("-d", "fish"), to::option_parse_error);

    to::option opt_e{e};
    ASSERT_NO_THROW(opt_e.run("", "bauble"));
    EXPECT_EQ("bauble", e);
}

