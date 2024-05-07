#include <iostream>
#include <iterator>
#include <vector>

#include <tinyopt/tinyopt.h>

const char* usage_str =
    "[OPTION] ...\n"
    "\n"
    "  -n [ INT [ INT [ INT ] ] ]  collect a vector of up to 3 integers\n"
    "  -h, --help                  display usage information and exit\n"
    "\n"
    "Collect and display vectors of up to 3 integers as multiple arguments to the -n option.\n";

int main(int argc, char** argv) {
    try {
        auto help = [argv0 = argv[0]] { to::usage(argv0, usage_str); };

        std::vector<std::vector<int>> nss;

        auto new_ns = [&]() { nss.push_back({}); return true; };
        auto push_ns = [&](int n) { nss.back().push_back(n); return true; };

        auto gt0 = [](int m) { return m>0; };
        auto decrement = [](int m) { return m-1; };

        to::option opts[] = {
            { to::action(help), "-h", "--help", to::flag, to::exit },
            { to::action(new_ns), to::then(3), "-n", to::flag },
            { to::action(push_ns), to::when(gt0), to::then(decrement)}
        };

        if (!to::run(opts, argc, argv+1)) return 0;
        if (argv[1]) throw to::option_error("unrecognized argument", argv[1]);

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
