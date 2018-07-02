#pragma once

#include <cstring>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <type_traits>
#include <vector>

#include <tinyopt/maybe.h>

namespace to {

struct parse_opt_error: public std::runtime_error {
    parse_opt_error(const std::string& s): std::runtime_error(s) {}
    parse_opt_error(const char *arg, const std::string& s):
        std::runtime_error(s+": "+arg) {}
};

void usage(const char* argv0, const std::string& usage_str) {
    const char* basename = std::strrchr(argv0, '/');
    basename = basename? basename+1: argv0;

    std::cout << "Usage: " << basename << " " << usage_str << "\n";
}

void usage(const char* argv0, const std::string& usage_str, const std::string& parse_err) {
    const char* basename = std::strrchr(argv0, '/');
    basename = basename? basename+1: argv0;

    std::cerr << basename << ": " << parse_err << "\n";
    std::cerr << "Usage: " << basename << " " << usage_str << "\n";
}

template <typename V>
struct default_parser {
    maybe<V> operator()(const std::string& text) const {
        V v;
        std::istringstream stream(text);
        stream >> v;
        return stream? maybe<V>(v): nothing;
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

    maybe<V> operator()(const std::string& text) const {
        for (const auto& p: map_) {
            if (text==p.first) return p.second;
        }
        return nothing;
    }
};

template <typename V>
class delimited_parser {
    char delim_;

public:
    explicit constexpr delimited_parser(char delim): delim_(delim) {}

    maybe<std::vector<V>> operator()(const std::string& text) const {
        std::vector<V> values;
        std::size_t p = 0, n = text.size();

        while (p<n) {
            std::size_t q = text.find(delim_, p);
            if (q==std::string::npos) q = n;

            V v;
            std::istringstream stream(text.substr(p, q-p));
            stream >> v;
            if (!stream) return nothing;

            values.push_back(std::move(v));
            p = q+1;
        }
        return values;
    }
};


template <typename V>
constexpr auto parse = default_parser<V>{};

template <typename KeywordPairs>
auto keywords(const KeywordPairs& pairs) {
    using std::begin;
    using value_type = std::decay_t<decltype(std::get<0>(*begin(pairs)))>;
    return keyword_parser<value_type>(pairs);
}

template <typename V, char delim>
constexpr auto delimited = delimited_parser<V>(delim);


template <typename V = std::string, typename P = default_parser<V>, typename = std::enable_if_t<!std::is_same<V, void>::value>>
maybe<V> parse_opt(char **& argp, char shortopt, const char* longopt=nullptr, const P& parse = P{}) {
    const char* arg = argp[0];

    if (!arg || arg[0]!='-') {
        return nothing;
    }

    std::string text;

    if (arg[1]=='-' && longopt) {
        const char* rest = arg+2;
        const char* eq = std::strchr(rest, '=');

        if (!std::strcmp(rest, longopt)) {
            if (!argp[1]) throw parse_opt_error(arg, "missing argument");
            text = argp[1];
            argp += 2;
        }
        else if (eq && !std::strncmp(rest, longopt,  eq-rest)) {
            text = eq+1;
            argp += 1;
        }
        else {
            return nothing;
        }
    }
    else if (shortopt && arg[1]==shortopt && arg[2]==0) {
        if (!argp[1]) throw parse_opt_error(arg, "missing argument");
        text = argp[1];
        argp += 2;
    }
    else {
        return nothing;
    }

    auto v = parse(text);

    if (!v) throw parse_opt_error(arg, "failed to parse option argument");
    return v;
}

maybe<void> parse_opt(char **& argp, char shortopt, const char* longopt) {
    if (!*argp || *argp[0]!='-') {
        return nothing;
    }
    else if (argp[0][1]=='-' && longopt && !std::strcmp(argp[0]+2, longopt)) {
        ++argp;
        return true;
    }
    else if (shortopt && argp[0][1]==shortopt && argp[0][2]==0) {
        ++argp;
        return true;
    }
    else {
        return nothing;
    }
}

} // namespace to
