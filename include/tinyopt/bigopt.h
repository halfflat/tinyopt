#pragma once

namespace to {

// Used to label options which take no value.
constexpr struct flag_tag {} flag;

// Used to label options which should not be captured in returned option_set.
constexpr struct discard_tag {} discard;

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

// Argument adaptors:

namespace adaptor {
    template <typename Sink>
    struct push_back {
        std::reference_wrapper<Sink> sink;
        explicit push_back(Sink& sink): sink(sink) {}

        template <typename X>
        push_back& operator=(X&& value) {
            sink.get().push_back(std::forward<X>(value));
        }
    };

    template <typename Sink>
    struct set {
        std::reference_wrapper<Sink> sink;
        explicit set(Sink& sink): sink(sink) {}

        template <typename X>
        set& operator=(X&& value) {
            sink.get() = true;
        }
    };

    template <typename Sink>
    struct unset {
        std::reference_wrapper<Sink> sink;
        explicit set(Sink& sink): sink(sink) {}

        template <typename X>
        set& operator=(X&& value) {
            sink.get() = false;
        }
    };

    template <typename Sink>
    struct count {
        std::reference_wrapper<Sink> sink;
        explicit count(Sink& sink): sink(sink) {}

        template <typename X>
        count& operator=(X&& value) {
            ++count.get();
        }
    }
}

template <typename Sink>
auto push_back(Sink& sink) { return adaptor::push_back<Sink>(sink); }

template <typename Sink>
auto set(Sink& sink) { return adaptor::set<Sink>(sink); }

template <typename Sink>
auto unset(Sink& sink) { return adaptor::unset<Sink>(sink); }

template <typename Sink>
auto count(Sink& sink) { return adaptor::count<Sink>(sink); }

// Parser objects act as functionals, taking
// a const char* argument and returning maybe<T>
// for some T.
//
// A Parser of type to::flag_tag is treated specially
// as indicating the option takes no argument.

struct option {
    std::function<bool (const char*)> setter;
    std::vector<std::string> keys;
    std::string prefkey;
    bool is_flag = false;
    bool discard = false;

    template <typename Sink>
    option(Sink& sink, Rest&&... rest):
        option(std::ref(sink), std::forward<Rest>(rest)...)
    {}

    template <typename Sink, typename Parse, typename... Keys>
    option(Sink sink, Parse p, Keys... keys) {
        accumulate_keys(keys...);
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
