#include <sstream>
#include <type_traits>
#include <gtest/gtest.h>

#include <tinyopt/tinyopt.h>

TEST(version, defines) {
#ifndef TINYOPT_VERSION
    FAIL() << "TINYOPT_VERSION undefined.";
#else
    auto version_str = TINYOPT_VERSION;
    ASSERT_TRUE((std::is_same<const char*, decltype(version_str)>::value));
#endif
#ifndef TINYOPT_VERSION_MAJOR
    FAIL() << "TINYOPT_VERSION_MAJOR undefined.";
#else
    auto version_major = TINYOPT_VERSION_MAJOR;
    ASSERT_TRUE((std::is_same<int, decltype(version_major)>::value));
#endif
#ifndef TINYOPT_VERSION_MINOR
    FAIL() << "TINYOPT_VERSION_MINOR undefined.";
#else
    auto version_minor = TINYOPT_VERSION_MINOR;
    ASSERT_TRUE((std::is_same<int, decltype(version_minor)>::value));
#endif
#ifndef TINYOPT_VERSION_PATCH
    FAIL() << "TINYOPT_VERSION_PATCH undefined.";
#else
    auto version_patch = TINYOPT_VERSION_PATCH;
    ASSERT_TRUE((std::is_same<int, decltype(version_patch)>::value));
#endif
#ifndef TINYOPT_VERSION_PRERELEASE
    FAIL() << "TINYOPT_VERSION_PRERELEASE undefined.";
#else
    auto prerelease_str = TINYOPT_VERSION_PRERELEASE;
    ASSERT_TRUE((std::is_same<const char*, decltype(prerelease_str)>::value));
#endif
}

TEST(version, consistent) {
#if defined(TINYOPT_VERSION) && defined(TINYOPT_VERSION_MAJOR) && defined(TINYOPT_VERSION_MINOR) && defined(TINYOPT_VERSION_PATCH) && defined(TINYOPT_VERSION_PRERELEASE)
    ASSERT_TRUE(TINYOPT_VERSION_MAJOR>=0);
    ASSERT_TRUE(TINYOPT_VERSION_MINOR>=0);
    ASSERT_TRUE(TINYOPT_VERSION_PATCH>=0);

    std::stringstream version_nopatch, version;
    version_nopatch << TINYOPT_VERSION_MAJOR << "." << TINYOPT_VERSION_MINOR;
    version << TINYOPT_VERSION_MAJOR << "." << TINYOPT_VERSION_MINOR << "." << TINYOPT_VERSION_PATCH;

    std::stringstream pre;
    pre << TINYOPT_VERSION_PRERELEASE;
    if (!pre.str().empty()) {
        version_nopatch << "-" << TINYOPT_VERSION_PRERELEASE;
        version << "-" << TINYOPT_VERSION_PRERELEASE;
    }

    if (TINYOPT_VERSION_PATCH>0) {
        ASSERT_EQ(version.str(), TINYOPT_VERSION);
    }
    else {
        ASSERT_TRUE(version_nopatch.str()==TINYOPT_VERSION || version.str()==TINYOPT_VERSION);
    }
#else
    FAIL() << "Missing version defines.";
#endif
}
