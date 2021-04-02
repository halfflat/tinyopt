#include <iostream>
#include <string>
#include <vector>

#include <tinyopt/tinyopt.h>

const char* usage_str =
    "[OPTION]... [ARGUMENT]...\n"
    "\n"
    "  -a, --apple    Print 'apple' but otherwise ignore.\n"
    "  --             Stop further argument processing.\n"
    "  -h, --help     Display usage information and exit.\n"
    "\n"
    "Throw away --apple options and report remaining arguments.\n";

int main(int argc, char** argv) {
    try {
        auto print_apple = [] { std::cout << "apple!\n"; };
        auto help = [argv0 = argv[0]] { to::usage(argv0, usage_str); };

        to::option opts[] = {
            { to::action(print_apple), to::flag, "-a", "--apple" },
            { to::action(help), to::flag, to::exit, "-h", "--help" },
            { {}, to::flag, to::stop, "--" }
        };

        if (!to::run(opts, argc, argv+1)) return 0;

        std::cout << "remaining arguments:";
        for (auto a = argv+1; *a; ++a) std::cout << ' ' << *a;
        std::cout << "\n";
    }
    catch (to::option_error& e) {
        to::usage_error(argv[0], usage_str, e.what());
        return 1;
    }
}
