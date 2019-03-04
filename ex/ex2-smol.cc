#include <numeric>
#include <iostream>
#include <vector>

#include <tinyopt/smolopt.h>

const char* usage_str =
    "[OPTION]...\n"
    "\n"
    "  --sum=N1,..,Nk   Sum the integers N1 through Nk.\n"
    "  -h, --help       Display usage information and exit\n";

int main(int argc, char** argv) {
    try {
        std::vector<int> x;
        auto help = [argv0 = argv[0]] { to::usage(argv0, usage_str); };

        to::option opts[] = {
            { {x, to::delimited<int>()}, "--sum" },
            { to::action(help), to::flag, to::exit, "-h", "--help" }
        };

        if (!to::run(opts, argc, argv)) return 0;
        std::cout << std::accumulate(x.begin(), x.end(), 0) << "\n";
    }
    catch (to::option_error& e) {
	to::usage(argv[0], usage_str, e.what());
	return 1;
    }
}
