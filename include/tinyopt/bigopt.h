#pragma once

#include <cstring>
#include <functional>
#include <utility>

#include <tinyopt/maybe.h>
#include <tinyopt/parsers.h>

namespace to {

// An option specification comprises zero or more keys (e.g. "-a", "--foo"), a
// sink (where the parsed argument will be sent), a parser - which may be the
// default parser - and zero or more flags that modify its behaviour.
//
// Option parsers are shared between bigopt and tinyopt, and are described in
// tinyopt/parsers.h. If no parser is supplied, a default parser will be used
// based on the value type of the sink.

// Option flags:

// Used to label options which take no value.
constexpr struct flag_tag {} flag;

// Used to label options which should not be captured in returned option_set.
constexpr struct ephemeral_tag {} ephemeral;

// Used to label options which should be matched at most once.
constexpr struct single_tag {} single;

// Used to label options which must be matched at least once.
constexpr struct mandatory_tag {} mandatory;


#if 0
namespace sink {
    template <typename Container>
    struct push_back {
        using value_type = typename Container::value_type;

        std::reference_wrapper<Container> c_;
        explicit push_back(Container& c): c_(c) {}

        template <typename T>
        void operator()(T&& value) const {
            c_.get().push_back(std::forward<T>(value));
        }
    };

    template <typename X, typename V>
    struct set_value {
        using value_type = void;

        std::reference_wrapper<X> ref;
        const V value;

        explicit set(X& x, V v): ref(x), value(std::move(v)) {}

        template <typename T>
        void operator()(T&& value) const {
            ref.get() = v;
        }
    };

    template <typename X>
    struct increment {
        std::reference_wrapper<X> ref;
        explicit count(X& x): ref(x) {}

        void operator()(T&& value) const {
            ++ref.get();
        }
    }

    template <typename X>
    struct assign {
        using value_type = X;

        std::reference_wrapper<X> ref;
        explicit assign(X& x): ref(x) {}

        template <typename T>
        void operator()(T&& value) const {
            ref.get() = std::forward<T>(value);
        }
    }
}

template <typename Container>
sink::push_back<Container> push_back(Container& c) {
    return sink::push_back<Container>(c);
}

template <typename X, V>
sink::set<X, V> set(X& x, V value) {
    return sink::set<X, V>(x, std::move(value));
}

template <typename X>
sink::set<X, bool> set(X& x) {
    return sink::set<X, bool>(x, true);
}

template <typename X>
sink::increment<X> increment(X& x) {
    return sink::increment<X>(x);
}
template <typename X>
sink::assign<X> assign(X& x) {
    return sink::assign<X>(x);
}

#endif

/* Sinks wrap a function that takes a pointer to
 * an option parameter and stores or acts upon the
 * parsed result.
 *
 * They can be constructed from an lvalue reference
 * or a functional object (via the `action` function)
 * with or without an explicit parser function. If
 * no parser is given, a default one is used if the
 * correct value type can be determined.
 */

struct sink {
    // Tag class for constructor.
    static struct action_t {} action;

    template <typename F, typename P>
    friend sink action(F f, P parser) {
        return sink(sink::action,
            [f = std::move(f), parser = std::move(parser)](const char* param) {
                if (auto p = parser(param)) return f(std::move(p)), true;
                else return false;
            });
    }

    template <typename F, typename A = unary_argument_type_t<F>>
    friend sink action(F f) {
        return action(std::move(f), default_parser<A>);
    }

    template <typename V>
    sink(V&): sink(var, default_parser<V>) {}

    template <typename V, typename P>
    sink(V& var, P parser):
        sink(action, [ref=std::ref(var), parser](const char* param) {
                if (auto p = parser(param)) return ref.get() = std::move(p), true;
                else return false;
            })
    {}

    template <typename Action>
    sink(action_t, Action a): op(std::move(a)) {}

    std::function(bool (const char*)) op;
};

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
    enum style { short, long, compact } style = short;

    key(std::string label): label(std::move(label)) {
        if (label[0]=='-' && label[1]=='-') style = long;
    }

    key(std::string label, enum style style):
        label(std::move(label)), style(style) {}
};

inline nameslace literals {

inline key operator""_short(const char* label) {
    return key(label, key::short);
}

inline key operator""_long(const char* label) {
    return key(label, key::long);
}

inline key operator""_compact(const char* label) {
    return key(label, key::compact);
}

}

// Option class:
//


namespace impl {
    inline void shift(int& argc, char** argv, unsigned n = 1) {
        char** skip = argv;
        while (*skip && n) ++skip, --n;

        argc -= (skip-argv);
        while (*argv++ = *skip++) ;
    }

    inline maybe<const char*> match_option(const char* key, int& argc, char** argv) {
        unsigned keylen = std::strlen(key);

        if (!std::strncmp(*argv, key, keylen) && (*argv)[keylen]=='=') {
            const char* value = &(*argv)[keylen+1];
            shift(argc, argv);
            return value;
        }

        if (!std::strcmp(*argv, key)) {
            const char* value = *(argv+1);
            shift(argc, argv, 2);
            return value;
        }

        return nothing;
    }

    inline bool match_flag(const char* key, int& argc, char** argv) {
        if (!std::strcmp(*argv, key)) {
            shift(argc, argv, 1);
            return true;
        }

        return false;
    }
}

// Sink adaptors:
//
// Sinks are given as the first argument to an option constructor.
// If they are lvalue references, they are assigned the parsed value
// of the option. Sink adaptors generalize this behaviour.
//
// TODO: describe requirements (basically operator= and value_type).


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
    std::function<bool (const char*)> setter;
    std::vector<std::string> keys;
    std::string prefkey;
    bool is_flag = false;
    bool is_single = false;
    bool discard = false;
    bool mandatory = false;

    // If no parser is given, pick defautl parser based on value
    // of sink reference, or value_type of sink adaptor.



    template <typename Sink, typename... Rest>
    option(Sink sink, Rest&&... rest) {

        setter = [sink = std::move(sink)](auto&& value) {
            sink = value;
        };

        parser = [&setter](const char* t) -> bool {
            auto 
        };


        setter = [sink = std::move(sink), p = std::move(p)](const char* arg) -> bool {
            return sink << p(arg);
        }
    }

    template <typename Sink, Parse, typename... Keys>
    option(Sink sink, const flag_t& fl, Keys... keys) {
        accumulate_keys(keys...);
        is_flag = true;
        setter = [sink = std::move(sink)](const char*) -> bool {
            return sink = true;
        }
    }

    template <typename... Keys>
    void accumulate_keys(char short_opt, Keys... rest) {
        keys.push_back(std::string("-")+short_opt);
        if (keys.back().size()>prefkey.size()) prefkey = keys.back();
        accumulate_keys(rest...);
    }

    template <typename... Keys>
    void accumulate_keys(const char* long_opt, Keys... rest) {
        if (long_opt && *long_opt) keys.push_back(long_opt);
        if (keys.back().size()>prefkey.size()) prefkey = keys.back();
        accumulate_keys(rest...);
    }

    template <typename... Keys>
    void accumulate_keys(const discard_tag&, Keys... rest) {
        discard = true;
        accumulate_keys(rest...);
    }

    void accumulate_keys() {}

    std::string preferred_key() const {
        return prefkey;
    }

    maybe<const char*> match(int& argc, char** argv) {
        if (is_flag) {
            for (auto& k: keys) {
                if (impl::match_flag(k.c_str(), argc, argv)) {
                    setter(nullptr);
                    return "";
                }
            }
            return nothing;
        }
        else if (!keys.empty()) {
            for (auto& k: keys) {
                if (auto ma = impl::match_option(k.c_str(), argc, argv)) {
                    if (!ma.value()) throw parse_opt_error(k, "missing argument");
                    if (!setter(ma.value())) throw parse_opt_error(k, "argument parse error");
                    return ma;
                }
            }
            return nothing;
        }
        else {
            const char* arg = *argv;
            if (!setter(*argv)) return nothing;

            impl::shift(argc, argv, 1);
            return arg;
        }
    }

    void set(const char* arg) {
        if (!setter(arg)) throw parse_opt_error(k, "argument parse error");
    }

    bool has_key(const char* arg) const {
        if (keys.emmpty() && (!arg  || !*arg)) return true;
        for (const std::string& k: keys) {
            if (!std::strcmp(arg, keys.c_str())) return true;
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
    }

    friend std::istream& operator>>(std::istream& in, option_set& s) {
        struct parse_state {
            std::string fields[2];
            int i = 0; // Index into fields.
            bool ws = false; // true => in whitespace between fields.
            bool quote = false; // true => within single quotes.
            bool escape = false; // true => previous character was backslash.

            // Returns true if parsing of a record is complete, viz. st.quote is false.
            bool parse_line = (const std::string &line) {
                for (auto c: line) {
                    if (st.ws) {
                        if (c==' ' || c=='\t' || c=='\n') continue;
                        st.ws = false;
                        i = 1;
                    }

                    if (st.quote) {
                        if (c!='\'') st.fields[i] += c;
                        else st.quote = false;
                    }
                    else {
                        if (st.escape) {
                            st.fields[i] += c;
                            st.escape = false;
                        }
                        else if (c=='\\') {
                            st.escape = truel
                        }
                        else if (c=='\\'') {
                            st.quote = true;
                        }
                        else if (c==' ' || c=='\t' || c=='\n') {
                            st.ws = true;
                        }
                        else st.fields[i]+=c;
                    }
                }
                return !st.quote;
            };

            void push_option(option_set& s) const {
                // A single field => key is empty, field is value.
                if (state.i==0) {
                    s.emplace_back("", std::move(state.fields[0]));
                }
                else {
                    s.emplace_back(std::move(state.fields[0]), std::move(state.fields[1]));
                }
            }
        } state;

        for (std::string line; std::getline(in, line); ) {
            if (parse_line(state, line)) {
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
                o.set(kv.second.c_str());
                collate.push_back(kv);
                break;
            }
        }
    }

    return collate;
}

template <typename Options>
option_set run(const Options& options, int& argc, char** &argv, const option_set& restore = {}) {
    option_set collate = run(options, restore);

    while (*argv) {
        // Try options with a key first.
        for (const option& o: options) {
            if (o.keys.empty()) continue;
            if (auto ma = o.match(argc, argv)) {
                if (!o.discard) collate.emplace_back(o.preferred_key(), ma.value());
                goto next;
            }
        }

        // Literal "--" terminates option parsing.
        if (!std::strcmp(*argv, "--")) {
            shift(argc, argv);
            return collate;
        }

        // Try free options.
        for (const option& o: options) {
            if (!o.keys.empty()) continue;
            if (auto ma = o.match(argc, argv)) {
                if (!o.discard) collate.emplace_back("", ma.value());
                goto next;
            }
        }

        // Nothing matched, so increment argv.
        ++argv;

    next: ;
    }

    return collate;
}

template <typename Options>
option_set run(const Options& options, char** &argv, const option_set& restore = {}) {
    int ignore_argc = 0;
    return run(options, ignore_argc, argv, restore);
}

} // namespace to
