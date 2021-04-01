#include <gtest/gtest.h>

#include <tinyopt/tinyopt.h>

using namespace to;

TEST(maybe, something) {
    constexpr auto s = something;
    EXPECT_TRUE(s);
    EXPECT_TRUE(s.has_value());

    constexpr auto n = nothing;
    EXPECT_FALSE(maybe<int>(nothing));
    EXPECT_FALSE(maybe<int>(nothing).has_value());
}

TEST(maybe, noexcept) {
    struct nontrivial {
        nontrivial() { throw 0; }
        nontrivial(const nontrivial&) { throw 0; }
        nontrivial(nontrivial&&) { throw 0; }
    };

    EXPECT_TRUE(noexcept(maybe<void>()));
    EXPECT_TRUE(noexcept(maybe<void>(1)));

    EXPECT_TRUE(noexcept(maybe<nontrivial>()));
    EXPECT_TRUE(noexcept(maybe<nontrivial>(nothing)));
}

struct count {
    count() { ++i; }
    ~count() { i -= !moved; }

    count(const count&) { ++c; ++i; }
    count(count&& x) { moved = x.moved; x.moved = true; ++m; }

    bool moved = false;

    static int i;
    static int c;
    static int m;
};

int count::i = 0;
int count::c = 0;
int count::m = 0;

TEST(maybe, ctor) {
    {
        maybe<count> m1;
        EXPECT_EQ(0, count::i);

        maybe<count> m2(nothing);
        EXPECT_EQ(0, count::i);

        maybe<count> m3(count{});
        EXPECT_EQ(1, count::i);
        EXPECT_EQ(1, count::m);
        EXPECT_EQ(0, count::c);

        maybe<count> m4(m3);
        EXPECT_EQ(2, count::i);
        EXPECT_EQ(1, count::m);
        EXPECT_EQ(1, count::c);
    }
    EXPECT_EQ(0, count::i);
}

TEST(maybe, conditional_assign) {
    int r = 0;

    EXPECT_TRUE(r << maybe<int>(0));
    EXPECT_EQ(0, r);

    EXPECT_TRUE(r << maybe<int>(4));
    EXPECT_EQ(4, r);

    EXPECT_TRUE(r << maybe<double>(5.3));
    EXPECT_EQ(5, r);

    EXPECT_FALSE(r << maybe<int>());
    EXPECT_EQ(5, r);
}

TEST(maybe, conditional_void_assign) {
    int r = 0;

    EXPECT_TRUE(r << something);
    EXPECT_EQ(1, r);

    EXPECT_FALSE(r << maybe<void>());
    EXPECT_EQ(1, r);
}

TEST(maybe, conditional_apply) {
    int c = 0;
    auto f = [&c]() { ++c; };

    EXPECT_TRUE(f << something);
    EXPECT_EQ(1, c);

    EXPECT_FALSE(f << nothing);
    EXPECT_EQ(1, c);

    EXPECT_TRUE(f << maybe<int>(22));
    EXPECT_EQ(2, c);

    EXPECT_FALSE(f << maybe<int>());
    EXPECT_EQ(2, c);
}

TEST(maybe, conditional_apply_arg) {
    int c = 0;
    auto f = [&c](int n) { c += n; };

    EXPECT_TRUE(f << maybe<int>(22));
    EXPECT_EQ(22, c);

    EXPECT_FALSE(f << maybe<int>());
    EXPECT_EQ(22, c);

    EXPECT_TRUE(f << maybe<double>(-7));
    EXPECT_EQ(15, c);

    c = 100;
    auto g = [&c](int n) { c += n; return n+1; };

    auto r1 = g << maybe<int>(10);
    ASSERT_TRUE(r1);
    EXPECT_EQ(11, r1.value());
    EXPECT_EQ(110, c);

    auto r2 = g << maybe<int>();
    EXPECT_FALSE(r2);
    EXPECT_EQ(110, c);
}

TEST(maybe, conv) {
    maybe<void> a(maybe<int>{});
    EXPECT_FALSE(a);

    maybe<void> b(maybe<int>{1});
    EXPECT_TRUE(b);

    maybe<double> c(maybe<int>{});
    EXPECT_FALSE(c);

    maybe<double> d(maybe<int>{1});
    EXPECT_TRUE(d);
    EXPECT_EQ(1., d.value());
}
