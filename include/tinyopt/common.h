#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <tinyopt/maybe.h>

namespace to {

// `option_error` is the base class for exceptions thrown
// by the option handling functions.

struct option_error: public std::runtime_error {
    option_error(const std::string& message): std::runtime_error(message) {}
    option_error(const std::string& message, std::string arg):
        std::runtime_error(message+": "+arg), arg(std::move(arg)) {}

    std::string arg;
};

struct option_parse_error: option_error {
    option_parse_error(const std::string &arg):
        option_error("option parse error", arg) {}
};

struct missing_mandatory_option: option_error {
    missing_mandatory_option(const std::string &arg):
        option_error("missing mandatory option", arg) {}
};

struct missing_argument: option_error {
    missing_argument(const std::string &arg):
        option_error("option misssing argument", arg) {}
};

struct unrecognized_option: option_error {
    unrecognized_option(const std::string &arg):
        option_error("unrecognized option", arg) {}
};

// `usage` prints usage information to stdout (no error message)
// or to stderr (with error message). It extracts the program basename
// from the provided argv[0] string.

inline void usage(const char* argv0, const std::string& usage_str) {
    const char* basename = std::strrchr(argv0, '/');
    basename = basename? basename+1: argv0;

    std::cout << "Usage: " << basename << " " << usage_str << "\n";
}

inline void usage(const char* argv0, const std::string& usage_str, const std::string& parse_err) {
    const char* basename = std::strrchr(argv0, '/');
    basename = basename? basename+1: argv0;

    std::cerr << basename << ": " << parse_err << "\n";
    std::cerr << "Usage: " << basename << " " << usage_str << "\n";
}

// Parser objects act as functionals, taking
// a const char* argument and returning maybe<T>
// for some T.

template <typename V>
struct default_parser {
    maybe<V> operator()(const char* text) const {
        if (!text) return nothing;
        V v;
        std::istringstream stream(text);
        stream >> v;
        return stream? maybe<V>(v): nothing;
    }
};

template <>
struct default_parser<void> {
    maybe<void> operator()(const char* text) const {
        return something;
    }
};

template <typename V>
class keyword_parser {
    std::vector<std::pair<std::string, V>> map_;

public:
    template <typename KeywordPairs>
    keyword_parser(const KeywordPairs& pairs) {
        using std::begin;
        using std::end;
        map_.assign(begin(pairs), end(pairs));
    }

    maybe<V> operator()(const char* text) const {
        if (!text) return nothing;
        for (const auto& p: map_) {
            if (text==p.first) return p.second;
        }
        return nothing;
    }
};

// Returns a parser that matches a set of keywords,
// returning the corresponding values in the supplied
// pairs.

template <typename KeywordPairs>
auto keywords(const KeywordPairs& pairs) {
    using std::begin;
    using value_type = std::decay_t<decltype(std::get<1>(*begin(pairs)))>;
    return keyword_parser<value_type>(pairs);
}


// A parser for delimited sequences of values; returns
// a vector of the values obtained from the supplied
// per-item parser.

template <typename P>
class delimited_parser {
    char delim_;
    P parse_;
    using inner_value_type = decltype(*std::declval<P>()(""));

public:
    template <typename Q>
    delimited_parser(char delim, Q&& parse): delim_(delim), parse_(std::forward<Q>(parse)) {}

    maybe<std::vector<inner_value_type>> operator()(const char* text) const {
        if (!text) return nothing;

        std::vector<inner_value_type> values;

        std::size_t n = std::strlen(text);
        std::vector<char> input(1+n);
        std::copy(text, text+n, input.data());

        for (char* p = input.data(); *p; ) {
            char* q = p;
            while (*q && *q!=delim_) ++q;
            *q++ = 0;

            if (auto mv = parse_(p)) values.push_back(*mv);
            else return nothing;

            p = q;
        }

        return values;
    }
};

// Convenience constructors for delimited parser.

template <typename Q>
auto delimited(char delim, Q&& parse) {
    using P = std::decay_t<Q>;
    return delimited_parser<Q>(delim, std::forward<Q>(parse));
}

template <typename V>
auto delimited(char delim = ',') {
    return delimited(delim, default_parser<V>{});
}


} // namespace to
