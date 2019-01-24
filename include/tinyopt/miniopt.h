#pragma once

#include <cstddef>
#include <cstring>
#include <functional>
#include <type_traits>
#include <utility>

#include <tinyopt/maybe.h>
#include <tinyopt/common.h>

namespace to {

// TODO: Arrange: state; sinks; keys; options; runner.

// An option specification comprises zero or more keys (e.g. "-a", "--foo"), a
// sink (where the parsed argument will be sent), a parser - which may be the
// default parser - and zero or more flags that modify its behaviour.
//
// Option parsers are shared between bigopt and tinyopt, and are described in
// tinyopt/parsers.h. If no parser is supplied, a default parser will be used
// based on the value type of the sink.

// Option flags:

enum option_flag {
    flag = 1,       // Option takes no parameter.
    ephemeral = 2,  // Option is not saved in returned results.
    single = 4,     // Option is parsed at most once.
    mandatory = 8   // Option must be present in argument list.
};

/* Sinks wrap a function that takes a pointer to an option parameter and
 * stores or acts upon the parsed result.
 *
 * They can be constructed from an lvalue reference or a functional
 * object (via the `action` function) with or without an explicit parser
 * function. If no parser is given, a default one is used if the correct
 * value type can be determined.
 */

namespace impl {
    template <typename T> struct fn_arg_type { using type = void; };
    template <typename R, typename X> struct fn_arg_type<R (X)> { using type = X; };
    template <typename R, typename X> struct fn_arg_type<R (*)(X)> { using type = X; };
    template <typename R, typename C, typename X> struct fn_arg_type<R (C::*)(X)> { using type = X; };
    template <typename R, typename C, typename X> struct fn_arg_type<R (C::*)(X) const> { using type = X; };
    template <typename R, typename C, typename X> struct fn_arg_type<R (C::*)(X) volatile> { using type = X; };
    template <typename R, typename C, typename X> struct fn_arg_type<R (C::*)(X) const volatile> { using type = X; };

    template <typename...> struct void_type { using type = void; };
}

template <typename T, typename = void>
struct unary_argument_type { using type = typename impl::fn_arg_type<T>::type; };

template <typename T>
struct unary_argument_type<T, typename impl::void_type<decltype(&T::operator())>::type> {
    using type = typename impl::fn_arg_type<decltype(&T::operator())>::type;
};

template <typename T>
using unary_argument_type_t = typename unary_argument_type<T>::type;

struct sink {
    // Tag class for constructor.
    static struct action_t {} action;

    template <typename V>
    sink(V& var): sink(var, default_parser<V>{}) {}

    template <typename V, typename P>
    sink(V& var, P parser):
        sink(action, [ref=std::ref(var), parser](const char* param) {
                if (auto p = parser(param)) return ref.get() = std::move(*p), true;
                else return false;
            })
    {}

    template <typename Action>
    sink(action_t, Action a): op(std::move(a)) {}

    bool operator()(const char* param) const { return op(param); }
    std::function<bool (const char*)> op;

};

// Convenience functions for construction of sink actions
// with explicit or implicit parser.

template <typename F, typename A = unary_argument_type_t<F>>
sink action(F f) {
    return sink(sink::action,
        [f = std::move(f)](const char* arg) -> bool {
            return f << default_parser<A>{}(arg);
        });
}

template <typename F, typename P>
sink action(F f, P parser) {
    return sink(sink::action,
        [f = std::move(f), parser = std::move(parser)](const char* arg) -> bool {
            return f << parser(arg);
        });
}

/* Sink adaptors:
 *
 * These adaptors constitute short cuts for making actions that
 * count the occurance of a flag, set a fixed value when a flag
 * is provided, or for appending an option parameter onto a vector
 * of values.
 */

// Push parsed option parameter on to container.
template <typename Container, typename P = default_parser<typename Container::value_type>>
sink push_back(Container& c, P parser = P{}) {
    return action(
        [ref = std::ref(c)](typename Container::value_type v) { ref.get().push_back(std::move(v)); },
        std::move(parser));
}

// Set v to value when option parsed; ignore any option parameter.
template <typename V>
sink set_value(V& v, V value) {
    return action([ref = std::ref(v), value = std::move(value)] { ref.get() = value; });
}

// Set v to true when option parsed; ignore any option parameter.
template <typename V>
sink set(V& v) {
    return set_value(v, true);
}

// Incrememnt v when option parsed; ignore any option parameter.
template <typename V>
sink increment(V& v) {
    return action([ref = std::ref(v)] { ++ref.get(); });
}


// Option keys:
//
// A key is how the option is specified in an argument list,
// and is typically represented as a 'short' (e.g. '-a')
// option or a 'long' option (e.g. '--apple').
//
// The value for an option can always be taken from the
// next argument in the list, but in addition can be specified
// together with the key itself, depending on the properties
// of the option key:
//
//     --key=value         'Long' style argument for key "--key"
//     -kvalue             'Compact' style argument for key "-k"
//
// Compact option keys can be combined in the one item in
// the argument list, if the options do not take any values
// (that is, they are glags). For example, if -a, -b are flags
// and -c takes an integer argument, with all three keys marked
// as compact, then an item '-abc3' in the argument list
// will be parsed in the same way as the sequence of items
// '-a', '-b', '-c', '3'.
//
// An option without a key will match any item in the argument
// list; options with keys are always checked first.

struct key {
    std::string label;
    enum style { shortfmt, longfmt, compact } style = shortfmt;

    key(std::string l): label(std::move(l)) {
        if (label[0]=='-' && label[1]=='-') style = longfmt;
    }

    key(const char* label): key(std::string(label)) {}

    key(std::string label, enum style style):
        label(std::move(label)), style(style) {}
};

inline namespace literals {

inline key operator""_short(const char* label, std::size_t) {
    return key(label, key::shortfmt);
}

inline key operator""_long(const char* label, std::size_t) {
    return key(label, key::longfmt);
}

inline key operator""_compact(const char* label, std::size_t) {
    return key(label, key::compact);
}

} // namespace literals

// Option parsing:
//


struct state {
    int& argc;
    char** argv;
    unsigned optoff = 0;

    state(int& argc, char** argv): argc(argc), argv(argv) {}

    explicit operator bool() const {
        return *argv;
    }

    void shift(unsigned n = 1) {
        char** skip = argv;
        while (*skip && n) ++skip, --n;

        argc -= (skip-argv);
        for (auto p = argv; *p; *p++ = *skip++) ;
        optoff = 0;
    }

    void skip() {
         if (*argv) ++argv;
    }

    const char* match_option(const key& k) {
        const char* p = nullptr;

        if (k.style==key::compact) {
            if ((p = match_compact_key(k.label.c_str()))) {
                if (!*p) {
                    p = argv[1];
                    shift(2);
                }
                else shift();
            }
        }
        else if (!optoff && k.label==*argv) {
            p = argv[1];
            shift(2);
        }
        else if (!optoff && k.style==key::longfmt) {
            auto keylen = k.label.length();
            if (!std::strncmp(*argv, k.label.c_str(), keylen) && (*argv)[keylen]=='=') {
                p = &(*argv)[keylen+1];
                shift();
            }
        }

        return p;
    }

    bool match_flag(const key& k) {
        if (k.style==key::compact) {
            if (auto p = match_compact_key(k.label.c_str())) {
                if (!*p) shift();
                return true;
            }
        }
        else if (!optoff && k.label==*argv) {
            shift();
            return true;
        }

        return false;
    }

    const char* match_compact_key(const char* k) {
        unsigned keylen = std::strlen(k);

        unsigned prefix_max = std::min(keylen-1, optoff);
        for (std::size_t l = 0; l<=prefix_max; ++l) {
            if (l && strncmp(*argv, k, l)) break;
            if (strncmp(*argv+optoff, k+l, keylen-l)) continue;
            optoff += keylen-l;
            return *argv+optoff;
        }

        return nullptr;
    }

};

// Option specification.
// Constructed with option(sink, [parser,] [flag | option-key, ...]).
//
// In argument processing, options with keys are tested first,
// followed by options without keys, which will match any argument.
//
// Option behaviour can be customized by providing a custom parser
// for the value, a sink adaptor for accepting the value, and by
// the following flags:
//
//     to::discard         Do not record this option value in the saved option set.
//     to::flag            Option takes no argument (sink is assigned with value 'true').
//     to::single          Option will be matched at most once in the argument list.
//
// Options are tested in the order they are supplied to to::run.
//
// The value of an option can be taken from the next argument in the argument list,
// provided with an '=' in the same argument. For example, for an option with keys "-f" and "--foo"
// will match any of:
//
//     -f value
//     -f=value
//     --foo value
//     --foo=value
//
// There is currently no support for combining flags into a single argument.

struct option {
    sink s;
    std::vector<key> keys;
    std::string prefkey;
    int count = 0;

    bool is_flag = false;
    bool is_ephemeral = false;
    bool is_single = false;
    bool is_mandatory = false;

    template <typename... Rest>
    option(sink s, Rest&&... rest): s(std::move(s)) {
        init_(std::forward<Rest>(rest)...);
    }

    void init_() {}

    template <typename... Rest>
    void init_(enum option_flag f, Rest&&... rest) {
        is_flag      |= f & flag;
        is_ephemeral |= f & ephemeral;
        is_single    |= f & single;
        is_mandatory |= f & mandatory;
        init_(std::forward<Rest>(rest)...);
    }

    template <typename... Rest>
    void init_(key k, Rest&&... rest) {
        if (k.label.length()>prefkey.length()) prefkey = k.label;
        keys.push_back(std::move(k));
        init_(std::forward<Rest>(rest)...);
    }

    std::string preferred_key() const {
        return prefkey;
    }

    maybe<const char*> match(state& st) {
        if (is_flag) {
            for (auto& k: keys) {
                if (st.match_flag(k)) {
                    set(k.label, nullptr);
                    return "";
                }
            }
            return nothing;
        }
        else if (!keys.empty()) {
            for (auto& k: keys) {
                if (auto param = st.match_option(k)) {
                    set(k.label, param);
                    return param;
                }
            }
            return nothing;
        }
        else {
            const char* param = *st.argv;
            st.shift();
            set("", param);
            return param;
        }
    }

    void set(const std::string& k, const char* arg) {
        cset(k, arg);
        ++count;
    }

    void cset(const std::string& k, const char* arg) const {
        if (!s(arg)) throw option_parse_error(k);
    }

    bool has_key(const std::string& arg) const {
        if (keys.empty() && arg.empty()) return true;
        for (const auto& k: keys) {
            if (arg==k.label) return true;
        }
        return false;
    }
};

struct option_set {
    std::vector<std::pair<std::string, std::string>> olist;

    decltype(olist.cbegin()) begin() const { return olist.begin(); }
    decltype(olist.cend()) end() const { return olist.end(); }

    void add(const std::string& key, const char* value) {
        olist.emplace_back(key, value? value: "");
    }

    // Serialized representation:
    //
    // Option entries separated by new lines, each option given as key then
    // value separated by white space (if there is a key) or simply value (if
    // the key is empty). The keys and values are escaped with single quotes in
    // a POSIX shell compatabile way, so that they can be used as is from the
    // shell.

    friend std::ostream& operator<<(std::ostream& out, const option_set& s) {
        auto escape = [](const std::string& v) {
            if (v.find_first_of("\\*?[#~=%|^;<>()$'`\" \t\n")==std::string::npos) return v;

            // Wrap string in single quotes, replacing any internal single quote
            // character with: '\''

            std::string q ="'";
            for (auto c: v) {
                q += c=='\''? "'\\''": v;
            }
            return q;
        };

        for (auto& p: s.olist) {
            if (!p.first.empty()) {
                out << escape(p.first) << ' '
                    << escape(p.second) << '\n';
            }
        }
        for (auto& p: s.olist) {
            if (p.first.empty()) {
                out << escape(p.second) << '\n';
            }
        }
        return out;
    }

    friend std::istream& operator>>(std::istream& in, option_set& s) {
        struct parse_state {
            std::string fields[2];
            int i = 0; // Index into fields.
            bool ws = false; // true => in whitespace between fields.
            bool quote = false; // true => within single quotes.
            bool escape = false; // true => previous character was backslash.

            // Returns true if parsing of a record is complete, viz. st.quote is false.
            bool parse_line(const std::string &line) {
                for (auto c: line) {
                    if (ws) {
                        if (c==' ' || c=='\t' || c=='\n') continue;
                        ws = false;
                        i = 1;
                    }

                    if (quote) {
                        if (c!='\'') fields[i] += c;
                        else quote = false;
                    }
                    else {
                        if (escape) {
                            fields[i] += c;
                            escape = false;
                        }
                        else if (c=='\\') {
                            escape = true;
                        }
                        else if (c=='\'') {
                            quote = true;
                        }
                        else if (c==' ' || c=='\t' || c=='\n') {
                            ws = true;
                        }
                        else fields[i]+=c;
                    }
                }
                return !quote;
            };

            void push_option(option_set& s) const {
                // A single field => key is empty, field is value.
                if (i==0) {
                    s.olist.emplace_back("", std::move(fields[0]));
                }
                else {
                    s.olist.emplace_back(std::move(fields[0]), std::move(fields[1]));
                }
            }
        } state;

        for (std::string line; std::getline(in, line); ) {
            if (state.parse_line(line)) {
                state.push_option(s);
                state = parse_state{};
            }
        }

        // If last line has an open quote, parse charitably.
        if (state.quote) state.push_option(s);

        return in;
    }
};

template <typename Options>
option_set run(const Options& options, const option_set& restore = {}) {
    option_set collate;

    for (auto& kv: restore) {
        for (const option& o: options) {
            if (o.has_key(kv.first)) {
                o.cset(kv.first, kv.second.c_str());
                collate.olist.push_back(kv);
                break;
            }
        }
    }

    return collate;
}

template <typename Options>
option_set run(const Options& options_in, int& argc, char** &argv, const option_set& restore = {}) {
    using std::begin;
    using std::end;
    std::vector<option> options(begin(options_in), end(options_in));

    state st{argc, argv};
    option_set collate = run(options, restore);

    while (st) {
        // Try options with a key first.
        for (option& o: options) {
            if (o.is_single && o.count) continue;
            if (o.keys.empty()) continue;
            if (auto ma = o.match(st)) {
                if (!o.is_ephemeral) collate.olist.emplace_back(o.preferred_key(), ma.value());
                goto next;
            }
        }

        // Literal "--" terminates option parsing.
        if (!std::strcmp(*argv, "--")) {
            st.shift();
            return collate;
        }

        // Try free options.
        for (option& o: options) {
            if (o.is_single && o.count) continue;
            if (!o.keys.empty()) continue;
            if (auto ma = o.match(st)) {
                if (!o.is_ephemeral) collate.olist.emplace_back("", ma.value());
                goto next;
            }
        }

        // Nothing matched, so increment argv.
        st.skip();

    next: ;
    }

    for (option& o: options) {
        if (o.is_mandatory && !o.count) throw missing_mandatory_option(o.prefkey);
    }

    return collate;
}

template <typename Options>
option_set run(const Options& options, char** &argv, const option_set& restore = {}) {
    int ignore_argc = 0;
    return run(options, ignore_argc, argv, restore);
}

} // namespace to
