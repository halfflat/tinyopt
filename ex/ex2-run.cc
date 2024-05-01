#include <numeric>
#include <iostream>
#include <vector>

#include <tinyopt/tinyopt.h>

const char* usage_str =
    "[OPTION]...\n"
    "\n"
    "  --sum=N1,..,Nk   sum the integers N1 through Nk\n"
    "  -h, --help       display usage information and exit\n";

int main(int argc, char** argv) {
    try {
        std::vector<int> x;
        auto help = [argv0 = argv[0]] { to::usage(argv0, usage_str); };

        to::option opts[] = {
            { {x, to::delimited<int>()}, "--sum" },
            { to::action(help), to::flag, to::exit, "-h", "--help" }
        };

        if (!to::run(opts, argc, argv+1)) return 0;
        if (argv[1]) throw to::option_error("unrecognized argument", argv[1]);

        std::cout << std::accumulate(x.begin(), x.end(), 0) << "\n";
    }
    catch (to::option_error& e) {
        to::usage_error(argv[0], usage_str, e.what());
        return 1;
    }
}
