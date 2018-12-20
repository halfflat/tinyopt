#pragma once

#include <cstring>
#include <functional>
#include <type_traits>
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
namespace impl {
>>>>>>> dff0272c168d1625207137460411592e3471badb:include/tinyopt/bigopt.h
    template <typename Container>
    struct push_back {
        using argument_type = typename Container::value_type;

        std::reference_wrapper<Container> c_;
        explicit push_back(Container& c): c_(c) {}

        template <typename T>
        void operator()(T&& value) const {
            c_.get().push_back(std::forward<T>(value));
        }
    };

    template <typename X, typename V>
    struct set_value {
<<<<<<< HEAD:include/tinyopt/miniopt.h
        using argument_type = void;
=======
        using value_type = void;
>>>>>>> dff0272c168d1625207137460411592e3471badb:include/tinyopt/bigopt.h

        std::reference_wrapper<X> ref;
        const V value;

        explicit set(X& x, V v): ref(x), value(std::move(v)) {}
        void operator()() const { ref.get() = v; }
    };

    template <typename X>
    struct increment {
<<<<<<< HEAD:include/tinyopt/miniopt.h
        using argument_type = void;

=======
>>>>>>> dff0272c168d1625207137460411592e3471badb:include/tinyopt/bigopt.h
        std::reference_wrapper<X> ref;

<<<<<<< HEAD:include/tinyopt/miniopt.h
        explicit count(X& x): ref(x) {}
        void operator()() const { ++ref.get(); }
=======
        void operator()(T&& value) const {
            ++ref.get();
        }
>>>>>>> dff0272c168d1625207137460411592e3471badb:include/tinyopt/bigopt.h
    }

    template <typename X>
    struct assign {
        using argument_type = X;

        std::reference_wrapper<X> ref;
        explicit assign(X& x): ref(x) {}

        template <typename T>
        void operator()(T&& value) const {
            ref.get() = std::forward<T>(value);
        }
    }

    template <typename S, typename A>
    using check_sink_sig =
        std::integral_constant<bo std::check_sing_sig_void<S, A>

    template <typename S>
    struct is_sink:
        std::integral_constant<bool,
            check_sink_sig_void<S>::value || check_sing



    

}

template <typename Container>
impl::push_back<Container> push_back(Container& c) {
    return impl::push_back<Container>(c);
}

template <typename X, V>
impl::set<X, V> set(X& x, V value) {
    return impl::set<X, V>(x, std::move(value));
}

template <typename X>
impl::set<X, bool> set(X& x) {
    return impl::set<X, bool>(x, true);
}

template <typename X>
impl::increment<X> increment(X& x) {
    return impl::increment<X>(x);
}

template <typename X>
impl::assign<X> assign(X& x) {
    return impl::assign<X>(x);
}

#endif

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

    template <typename...> struct void_type { using type = void; };
}

template <typename T, typename = void>
struct unary_argument_type { using type = typename impl::fn_arg_type<T>::type; };

template <typename T>
struct unary_argument_type<T, typename impl::void_type<decltype(&T::operator())>::type> {
    using type = typename impl::fn_arg_type<decltype(&T::operator())>::type;
};

template <typename T>
using unary_argument_type_t = typename impl::unary_argument_type<T>::type;

struct sink {
    // Tag class for constructor.
    static struct action_t {} action;

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

    // Convenience functions for construction of sink actions
    // with explicit or implicit parser.

    template <typename F, typename A = unary_argument_type_t<F>>
    friend sink action(F f) {
        return sink(sink::action,
            [f = std::move(f)](const char* arg) -> bool {
                return f << default_parser<A>{}(arg);
            });
    }

    template <typename F, typename P>
    friend sink action(F f, P parser) {
        return sink(sink::action,
            [f = std::move(f), parser = std::move(parser)](const char* arg) -> bool {
                return f << parser(arg);
            });
    }

};

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
    enum style { shortfmt, longfmt, compact } style = short;

    key(std::string label): label(std::move(label)) {
        if (label[0]=='-' && label[1]=='-') style = long;
    }

    key(std::string label, enum style style):
        label(std::move(label)), style(style) {}
};

inline nameslace literals {

inline key operator""_short(const char* label) {
    return key(label, key::shortfmt);
}

inline key operator""_long(const char* label) {
    return key(label, key::longfmt);
}

inline key operator""_compact(const char* label) {
    return key(label, key::compact);
}

}

// Option parsing:
//


struct state {
    int& argc;
    char** argv;
    char* optend = nullptr;

    state(int& argc, char** argv): argc(argc), argv(argv) {}

    void shift(unsigned n = 1) {
        char** skip = argv;
        while (*skip && n) ++skip, --n;

        argc -= (skip-argv);
        while (*argv++ = *skip++) ;
        optend = nullptr;
    }

    const char* match_option(const key& k) {
        unsigned keylen = k.key.length();
        const char* p = nullptr;

        if (k.key==*argv) {
            p = argv[1];
            shift(2);
        }
        else if (k.style==key::longfmt && !std::strncmp(*argv, k.key.c_str(), keylen) && (*argv)[keylen]=='=') {
            p = &(*st.argv)[keylen+1];
            shift();
        }
        else if (k.style==key::compact) {
            if (!optend && !std::strcmp(*argv, k.key.c_str(), keylen)) {
                p = &(*st.argv)[keylen];
            }
            else {
                std::ptrdiff_t prefix_lim = std::min(keylen, optend-*argv);
                for (std::ptrdiff_t l = 0; l<prefix_lim; ++l) {
                    if (strncmp(*argv, k.key.c_str(), l)) break;
                    if (strncmp(optend, k.key.c_str()+l, keylen-l)) continue;
                    p = optend+(keylen-l);
                }
            }
            if (p) {
                if (!*p) {
                    p = argv[1];
                    shift(2);
                }
                else shift();
            }
        }

        return p;
    }

    bool match_flag(const key& k) {
        if (k.key==*argv) 
        if (!std::strcmp(*st.argv, key)) {
            shift(argc, argv, 1);
            return true;
        }

        return false;
    }

};

    inline bool match_flag(const char* key, state& st) P
        if (!std::strcmp(*st.argv, key)) {
            shift(argc, argv, 1);
            return true;
        }

        return false;
    }
}

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
