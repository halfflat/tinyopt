#include <iostream>
#include <string>
#include <vector>

#include <tinyopt/tinyopt.h>

const char* usage_str =
    "[OPTION]... [ARGUMENT]...\n"
    "\n"
    "  -a, --apple    print 'apple'\n"
    "\n"
    "  --             stop further argument processing\n"
    "  -h, --help     display usage information and exit\n"
    "\n"
    "Disregarding --apple options, report remaining arguments.\n";

int main(int, char** argv) {
    try {
        std::vector<std::string> remaining;
        auto print_apple = [] { std::cout << "apple!\n"; };
        bool help = false;
        bool stop = false;

        char** arg = argv+1;
        while (*arg && !help && !stop) {
            bool match =
                help        << to::parse(arg, "-h", "--help") ||
                print_apple << to::parse(arg, "-a", "--apple") ||
                stop        << to::parse(arg, "--");

            if (!match) remaining.push_back(*arg++);
        }

        if (help) {
            to::usage(argv[0], usage_str);
            return 0;
        }

        // Grab any remaining unprocessed arguments, too.
        while (*arg) remaining.push_back(*arg++);

        std::cout << "remaining arguments:";
        for (auto& a: remaining) std::cout << ' ' << a;
        std::cout << "\n";
    }
    catch (to::option_error& e) {
        to::usage_error(argv[0], usage_str, e.what());
        return 1;
    }
}
