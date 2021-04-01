#include <string>
#include <utility>
#include <tinyopt/tinyopt.h>

const char* usage_str =
    "[OPTION]...\n"
    "\n"
    "  -n, --number=N       Specify N\n"
    "  -f, --function=FUNC  Perform FUNC, which is one of: one, two\n"
    "  -h, --help           Display usage information and exit\n";

int main(int argc, char** argv) {
    try {
        int n = 1, fn = 0;

        std::pair<const char*, int> functions[] = {
            { "one", 1 }, { "two", 2 }
        };

        auto help = [argv0 = argv[0]] { to::usage(argv0, usage_str); };

        to::option opts[] = {
            { n, "-n", "--number" },
            { {fn, to::keywords(functions)}, "-f", "--function", to::mandatory },
            { to::action(help), to::flag, to::exit, "-h", "--help" }
        };

        if (!to::run(opts, argc, argv+1)) return 0;

        if (argv[1]) throw to::option_error("unrecogonized argument", argv[1]);
        if (n<1) throw to::option_error("N must be at least 1");

        // Do things with arguments:

        for (int i = 0; i<n; ++i) {
            std::cout << "Performing function #" << fn << "\n";
        }
    }
    catch (to::option_error& e) {
        to::usage_error(argv[0], usage_str, e.what());
        return 1;
    }
}
