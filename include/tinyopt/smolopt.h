#pragma once

// Small option parsing/handling, but no so teeny as tinyopt.

#include <cstddef>
#include <cstring>
#include <functional>
#include <type_traits>
#include <utility>

#include <tinyopt/maybe.h>
#include <tinyopt/common.h>

namespace to {

// Option keys
// -----------
//
// A key is how the option is specified in an argument list, and is typically
// represented as a 'short' (e.g. '-a') option or a 'long' option (e.g.
// '--apple').
//
// The value for an option can always be taken from the next argument in the
// list, but in addition can be specified together with the key itself,
// depending on the properties of the option key:
//
//     --key=value         'Long' style argument for key "--key"
//     -kvalue             'Compact' style argument for key "-k"
//
// Compact option keys can be combined in the one item in the argument list, if
// the options do not take any values (that is, they are flags). For example,
// if -a, -b are flags and -c takes an integer argument, with all three keys
// marked as compact, then an item '-abc3' in the argument list will be parsed
// in the same way as the sequence of items '-a', '-b', '-c', '3'.
//
// An option without a key will match any item in the argument list; options
// with keys are always checked first.

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

// Argument state
// --------------
//
// to::state represents the collection of command line arguments. Mutating
// operations (shift(), successful option matching, etc.) will modify the
// underlying set of arguments used to construct the state object.

struct state {
    int& argc;
    char** argv;
    unsigned optoff = 0;

    state(int& argc, char** argv): argc(argc), argv(argv) {}

    // False => no more arguments.
    explicit operator bool() const { return *argv; }

    // Shift arguments left in-place.
    void shift(unsigned n = 1) {
        char** skip = argv;
        while (*skip && n) ++skip, --n;

        argc -= (skip-argv);
        auto p = argv;
        do { *p++ = *skip; } while (*skip++);

        optoff = 0;
    }

    // Skip current argument without modifying list.
    void skip() {
         if (*argv) ++argv;
    }

    // Match an option given by the key which takes an argument.
    // If successful, consume option and argument and return pointer to
    // argument string, else return nullptr.
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

    // Match a flag given by the key.
    // If successful, consume flag and return true, else return false.
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

    // Compact-style keys can be combined in one argument; combined keys
    // with a common prefix only need to supply the prefix once at the
    // beginning of the argument.
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

// Sinks and actions
// -----------------
//
// Sinks wrap a function that takes a pointer to an option parameter and stores
// or acts upon the parsed result.
//
// They can be constructed from an lvalue reference or a functional object (via
// the `action` function) with or without an explicit parser function. If no
// parser is given, a default one is used if the correct value type can be
// determined.

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
            return static_cast<bool>(f << default_parser<A>{}(arg));
        });
}

template <typename F, typename P>
sink action(F f, P parser) {
    return sink(sink::action,
        [f = std::move(f), parser = std::move(parser)](const char* arg) -> bool {
            return static_cast<bool>(f << parser(arg));
        });
}

// Sink adaptors:
//
// These adaptors constitute short cuts for making actions that count the
// occurance of a flag, set a fixed value when a flag is provided, or for
// appending an option parameter onto a vector of values.

// Push parsed option parameter on to container.
template <typename Container, typename P = default_parser<typename Container::value_type>>
sink push_back(Container& c, P parser = P{}) {
    return action(
        [ref = std::ref(c)](typename Container::value_type v) { ref.get().push_back(std::move(v)); },
        std::move(parser));
}

// Set v to value when option parsed; ignore any option parameter.
template <typename V, typename X>
sink set(V& v, X value) {
    return action([ref = std::ref(v), value = std::move(value)] { ref.get() = value; });
}

// Set v to true when option parsed; ignore any option parameter.
template <typename V>
sink set(V& v) {
    return set(v, true);
}

// Incrememnt v when option parsed; ignore any option parameter.
template <typename V>
sink increment(V& v) {
    return action([ref = std::ref(v)] { ++ref.get(); });
}

template <typename V, typename X>
sink increment(V& v, X delta) {
    return action([ref = std::ref(v), delta = std::move(delta)] { ref.get() += delta; });
}


// Option specification
// --------------------
//
// An option specification comprises zero or more keys (e.g. "-a", "--foo"),
// a sink (where the parsed argument will be sent), a parser - which may be
// the default parser - and zero or more flags that modify its behaviour.
//
// Option flags:

enum option_flag {
    flag = 1,       // Option takes no parameter.
    ephemeral = 2,  // Option is not saved in returned results.
    single = 4,     // Option is parsed at most once.
    mandatory = 8,  // Option must be present in argument list.
    exit = 16,      // Option stops further argument processing.
};

struct option {
    sink s;
    std::vector<key> keys;
    std::string prefkey;

    bool is_flag = false;
    bool is_ephemeral = false;
    bool is_single = false;
    bool is_mandatory = false;
    bool is_exit = false;

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
        is_exit      |= f & exit;
        init_(std::forward<Rest>(rest)...);
    }

    template <typename... Rest>
    void init_(key k, Rest&&... rest) {
        if (k.label.length()>prefkey.length()) prefkey = k.label;
        keys.push_back(std::move(k));
        init_(std::forward<Rest>(rest)...);
    }

    bool has_key(const std::string& arg) const {
        if (keys.empty() && arg.empty()) return true;
        for (const auto& k: keys) {
            if (arg==k.label) return true;
        }
        return false;
    }

    void run(const key& k, const char* arg) const {
        if (!is_flag && !arg) throw missing_argument(k.label);
        if (!s(arg)) throw option_parse_error(k.label);
    }
};

// Saved options
// -------------
//
// Successfully matched options, excluding those with the to::ephemeral flag
// set, are collated in a saved_options structure for potential documentation
// or replay.

struct saved_options: private std::vector<std::string> {
    using std::vector<std::string>::begin;
    using std::vector<std::string>::end;
    using std::vector<std::string>::size;
    using std::vector<std::string>::empty;

    void add(std::string s) { push_back(std::move(s)); }

    saved_options& operator+=(const saved_options& so) {
        insert(end(), so.begin(), so.end());
        return *this;
    }

    struct arglist {
        int argc;
        char** argv;
        std::vector<char*> arg_data;
    };

    // Construct argv representing argument list.
    arglist as_arglist() const {
        arglist A;

        for (auto& a: *this) A.arg_data.push_back(const_cast<char*>(a.c_str()));
        A.arg_data.push_back(nullptr);
        A.argv = A.arg_data.data();
        A.argc = A.arg_data.size()-1;
        return A;
    }

    // Serialized representation:
    //
    // Saved arguments are separated by white space. If an argument
    // contains whitespace or a special character, it is escaped with
    // single quotes in a POSIX shell compatible fashion, so that
    // the representation can be used directly on a shell command line.

    friend std::ostream& operator<<(std::ostream& out, const saved_options& s) {
        auto escape = [](const std::string& v) {
            if (!v.empty() && v.find_first_of("\\*?[#~=%|^;<>()$'`\" \t\n")==std::string::npos) return v;

            // Wrap string in single quotes, replacing any internal single quote
            // character with: '\''

            std::string q ="'";
            for (auto c: v) {
                c=='\''? q += "'\\''": q += c;
            }
            return q += '\'';
        };

        bool first = true;
        for (auto& p: s) {
            if (first) first = false; else out << ' ';
            out << escape(p);
        }
        return out;
    }

    friend std::istream& operator>>(std::istream& in, saved_options& s) {
        std::string w;
        bool have_word = false;
        bool quote = false; // true => within single quotes.
        bool escape = false; // true => previous character was backslash.
        while (in) {
            char c = in.get();
            if (c==EOF) break;

            if (quote) {
                if (c!='\'') w += c;
                else quote = false;
            }
            else {
                if (escape) {
                    w += c;
                    escape = false;
                }
                else if (c=='\\') {
                    escape = true;
                    have_word = true;
                }
                else if (c=='\'') {
                    quote = true;
                    have_word = true;
                }
                else if (c!=' ' && c!='\t' && c!='\n') {
                    w += c;
                    have_word = true;
                }
                else {
                    if (have_word) s.add(w);
                    have_word = false;
                    w = "";
                }
            }
        }

        if (have_word) s.add(w);
        return in;
    }
};

// Option with mutable state (for checking single and mandatory flags),
// used by to::run().

struct option_state: option {
    int count = 0;

    option_state(const option& o): option(o) {}

    // On successful match, return pointers to matched key and value.
    // For flags, use nullptr for value.
    maybe<std::pair<const char*, const char*>> match(state& st) {
        if (is_flag) {
            for (auto& k: keys) {
                if (st.match_flag(k)) return set(k, nullptr);
            }
            return nothing;
        }
        else if (!keys.empty()) {
            for (auto& k: keys) {
                if (auto param = st.match_option(k)) return set(k, param);
            }
            return nothing;
        }
        else {
            const char* param = *st.argv;
            st.shift();
            return set("", param);
        }
    }

    std::pair<const char*, const char*> set(const key& k, const char* arg) {
        run(k, arg);
        ++count;
        return {k.label.c_str(), arg};
    }
};

// Running a set of options
// ------------------------
//
// to::run() can be used to parse options from the command-line and/or from
// saved_options data.
//
// The first argument is a collection or sequence of option specifications,
// followed optionally by command line argc and argv or just argv. A
// saved_options object can be optionally passed as the last parameter.
//
// If an option with the to::exit flag is matched, option parsing will
// immediately stop and an empty value will be returned. Otherwise to::run()
// will return a saved_options structure recording the successfully parsed
// options.

template <typename Options>
maybe<saved_options> run(const Options& options, int& argc, char** argv) {
    using std::begin;
    using std::end;
    std::vector<option_state> opts(begin(options), end(options));

    saved_options collate;
    bool exit = false;
    state st{argc, argv};
    while (st && !exit) {
        // Try options with a key first.
        for (auto& o: opts) {
            if (o.is_single && o.count) continue;
            if (o.keys.empty()) continue;
            if (auto ma = o.match(st)) {
                if (!o.is_ephemeral) {
                    collate.add(ma->first);
                    if (ma->second) collate.add(ma->second);
                }
                exit = o.is_exit;
                goto next;
            }
        }

        // Literal "--" terminates option parsing.
        if (!std::strcmp(*argv, "--")) {
            st.shift();
            return collate;
        }

        // Try free options.
        for (auto& o: opts) {
            if (o.is_single && o.count) continue;
            if (!o.keys.empty()) continue;
            if (auto ma = o.match(st)) {
                if (!o.is_ephemeral) collate.add(ma->second);
                exit = o.is_exit;
                goto next;
            }
        }

        // Nothing matched, so increment argv.
        st.skip();
    next: ;
    }

    if (exit) return nothing;

    for (auto& o: opts) {
        if (o.is_mandatory && !o.count) throw missing_mandatory_option(o.prefkey);
    }
    return collate;
}

template <typename Options>
maybe<saved_options> run(const Options& options, const saved_options& restore) {
    auto A = restore.as_arglist();
    return run(options, A.argc, A.argv);
}

template <typename Options>
maybe<saved_options> run(const Options& options, int& argc, char** argv, const saved_options& restore) {
    saved_options coll1, coll2;

    if (coll1 << run(options, restore) && coll2 << run(options, argc, argv)) {
        coll1 += coll2;
        return coll1;
    }

    return nothing;
}

template <typename Options>
maybe<saved_options> run(const Options& options, char** argv) {
    int ignore_argc = 0;
    return run(options, ignore_argc, argv);
}

template <typename Options>
maybe<saved_options> run(const Options& options, char** argv, const saved_options& restore) {
    int ignore_argc = 0;
    return run(options, ignore_argc, argv, restore);
}
} // namespace to
