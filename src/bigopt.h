#pragma once

namespace to {

struct flag_t {};

constexpr struct set_t: flag_t {
    template <typename Sink>
    void operator()(Sink& sink) const { sink = true; }
} set;

constexpr struct unset_t: flag_t {
    template <typename Sink>
    void operator()(Sink& sink) const { sink = false; }
} unset;

constexpr struct count_t: flag_t {
    template <typename Sink>
    void operator()(Sink& sink) const { ++sink; }
} count;

constexpr struct discard_t {}; discard;

template <typename Sink>
struct push_back_t {
    Sink& sink;
    explicit push_back_t(Sink& sink): sink(sink) {}

    template <typename X>
    push_back_t& operator=(X&& value) {
    }

template <typename Sink>
auto push_back(Sink& sink) {
    return push_back_t<Sink>(sink);
}

struct option {
    std::function<bool (const char*)> setter;
    std::vector<std::string> keys;
    bool is_flag;
    bool discard = false;

    template <typename Sink, typename Parse, typename... Keys>
    option(Sink& sink, Parse p, Keys... keys) {
        accumulate_keys(keys...);
        flag = false;
        setter = [&sink, p](const char* arg) -> bool {
            auto maybe_value = p(arg);
            return sink << p(arg);
        }
    }

    template <typename Sink, Parse, typename... Keys>
    option(Sink& sink, const flag& fl, Keys... keys) {
        accumulate_keys(keys...);
        is_flag = true;
        setter = [&sink, fl](const char*) -> bool {
            fl(sink);
            return true;
        }
    }

    template <typename... Keys>
    void accumulate_keys(char short_opt, Keys... rest) {
        keys.push_back(std::string("-")+short_opt);
        accumulate_keys(rest...);
    }

    template <typename... Keys>
    void accumulate_keys(const char* long_opt, Keys... rest) {
        keys.push_back(std::string("--")+long_opt);
        accumulate_keys(rest...);
    }

    template <typename... Keys>
    void accumulate_keys(const discard_t&, Keys... rest) {
        discard = true;
        accumulate_keys(rest...);
    }

    void accumulate_keys() {}
};

using option_set = std::vector<std::pair<std::string,std::string>>;

template <typename Options>
option_set run(const Options& options, const option_set& restore = {}) {
    return {};
}

template <typename Options>
option_set run(const Options& options, int& argc, char** &argv, const option_set& restore = {}) {
    option_set collate = run(options, restore);

    while (*argv) {
        for (const option& o: options) {
            if (o.keys.empty()) continue;
            
        }


        for (const option& o: options) {
            if (!o.keys.empty()) continue;

        }
    }

    return collate;
}

template <typename Options>
option_set run(const Options& options, char** &argv, const option_set& restore = {}) {
    int ignore_argc = 0;
    return run(options, ignore_argc, argv, restore);
}

} // namespace to
