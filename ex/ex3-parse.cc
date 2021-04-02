#include <iostream>
#include <string>
#include <vector>

#include <tinyopt/tinyopt.h>

const char* usage_str =
    "[OPTION]... [ARGUMENT]...\n"
    "\n"
    "  -a, --apple    Print 'apple' but otherwise ignore.\n"
    "  -h, --help     Display usage information and exit\n"
    "\n"
    "Throw away --apple options and report remaining arguments.\n";

int main(int, char** argv) {
    try {
        std::vector<std::string> remaining;
        auto print_apple = [] { std::cout << "apple!\n"; };
        bool help = false;

        for (auto arg = argv+1; *arg; ) {
            bool match =
                help        << to::parse(arg, "-h", "--help") ||
                print_apple << to::parse(arg, "-a", "--apple");

            if (!match) remaining.push_back(*arg++);
        }

        if (help) {
            to::usage(argv[0], usage_str);
            return 0;
        }

        std::cout << "remaining arguments:";
        for (auto& a: remaining) std::cout << ' ' << a;
        std::cout << "\n";
    }
    catch (to::option_error& e) {
        to::usage_error(argv[0], usage_str, e.what());
        return 1;
    }
}
