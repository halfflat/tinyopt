#pragma once

// Parser objects act as functionals, taking
// a const char* argument and returning maybe<T>
// for some T.

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <tinyopt/maybe.h>

namespace to {

template <typename V>
struct default_parser {
    maybe<V> operator()(const char* text) const {
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
    using value_type = std::decay_t<decltype(std::get<0>(*begin(pairs)))>;
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
        std::vector<inner_value_type> values;

        std::size_t n = std::strlen(text);
        std::vector<char> input(1+n);
        std::copy(text, text+n, input.data());

        for (const char* p = input.data(); *p; ) {
            const char* q = p;
            while (*q && q!=delim_) ++q;
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
