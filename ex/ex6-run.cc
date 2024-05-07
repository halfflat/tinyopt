#include <iostream>
#include <string>

#include <tinyopt/tinyopt.h>

const char* usage_str =
    "[OPTIONS]...\n"
    "\n"
    "  -n fish | cake     print a message indicating a keyword argument\n"
    "  -n INT             print a message indicating aninteger argument\n"
    "  -n                 print a message indicating no argument\n"
    "  -h, --help         display usage information and exit\n";

void print_kw(const char* kw) {
    std::cout << "keyword argument: " << kw << "\n";
}

void print_int(int n) {
    std::cout << "integer argument: " << n << "\n";
}

void print_flag() {
    std::cout << "no argument\n";
}

int main(int argc, char** argv) {
    try {
        std::pair<const char*, const char*> kw_tbl[] = {
            { "fish", "FISH" }, { "cake", "CAKE" }
        };

        auto help = [argv0 = argv[0]] { to::usage(argv0, usage_str); };

        to::option opts[] = {
            { to::action(help), "-h", "--help" },
            { to::action(print_kw, to::keywords(kw_tbl)), "-n", to::lax },
            { to::action(print_int, to::default_parser<int>{}), "-n", to::lax },
            { to::action(print_flag), "-n", to::flag },
        };

        to::run(opts, argc, argv+1);
        if (argv[1]) throw to::option_error("unrecognized argument", argv[1]);
    }
    catch (to::option_error& e) {
        to::usage_error(argv[0], usage_str, e.what());
        return 1;
    }
}
