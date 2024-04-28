#include <iostream>
#include <iterator>
#include <vector>

#include <tinyopt/tinyopt.h>

const char* usage_str =
    "[-n [ INT [ INT [ INT ] ] ] | -a | -b ] ...\n"
    "\n"
    "Collect vectors of up to 3 integers as multiple arguments to the -n option.\n"
    "Count occurances of flags -a and -b.\n";

int main(int argc, char** argv) {
    try {
        auto help = [argv0 = argv[0]] { to::usage(argv0, usage_str); };

        std::vector<std::vector<int>> nss;
        auto new_ns = [&]() { nss.push_back({}); return true; };
        auto push_ns = [&](int n) { nss.back().push_back(n); return true; };

        int a = 0, b = 0;

#if 0
        to::option opts[] = {
            { to::action(help), "-h", "--help", to::exit },
            { to::increment(a), to::then(0), "-a", to::flag },
            { to::increment(b), to::then(0), "-b", to::flag },
            { to::action(new_ns), to::then(1), "-n", to::flag },
            { to::action(push_ns), to::when(1) }
        };
#endif
        auto gt0 = [](int m) { return m>0; };
        auto decrement = [](int m) { return m-1; };

        to::option opts[] = {
            { to::action(help), "-h", "--help", to::exit },
            { to::increment(a), to::then(0), "-a", to::flag },
            { to::increment(b), to::then(0), "-b", to::flag },
            { to::action(new_ns), to::then(3), "-n", to::flag },
            { to::action(push_ns), to::when(gt0), to::then(decrement)}
        };

        if (!to::run(opts, argc, argv+1)) return 0;
        if (argv[1]) throw to::option_error("unrecogonized argument", argv[1]);

        std::cout << "a count: " << a << "\nb count: " << b << "\n";
        std::cout << "ns:\n";
        for (auto& ns: nss) {
            std::cout << "{ ";
            std::ostream_iterator<int> os(std::cout, " ");
            for (int n: ns) *os = n;
            std::cout << "}\n";
        }
    }
    catch (to::option_error& e) {
        to::usage_error(argv[0], usage_str, e.what());
        return 1;
    }
}
