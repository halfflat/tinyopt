#include <numeric>
#include <iostream>
#include <vector>

#include <tinyopt/tinyopt.h>

const char* usage_str =
    "[OPTION]...\n"
    "\n"
    "  --sum=N1,..,Nk   Sum the integers N1 through Nk.\n"
    "  -h, --help       Display usage information and exit\n";

int main(int, char** argv) {
    try {
        std::vector<int> x;
        bool help = false;

        for (auto arg = argv+1; *arg; ) {
            bool ok =
                help << to::parse(arg, "-h", "--help") ||
                x    << to::parse<std::vector<int>>(arg, to::delimited<int>(), "--sum");

            if (!ok) throw to::option_error("unrecognized argument", *arg);
        }

        if (help) {
            to::usage(argv[0], usage_str);
            return 0;
        }

        std::cout << std::accumulate(x.begin(), x.end(), 0) << "\n";
    }
    catch (to::option_error& e) {
        to::usage_error(argv[0], usage_str, e.what());
        return 1;
    }
}
