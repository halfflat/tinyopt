#include <string>
#include <utility>
#include <tinyopt/tinyopt.h>

const char* usage_str =
    "[OPTION]...\n"
    "\n"
    "  -n, --number=N       specify number of times to perform function\n"
    "  -f, --function=FUNC  specify function, which is one of: one, two;\n"
    "                       this option is mandatory\n"
    "\n"
    "  -h, --help           display usage information and exit\n";

int main(int, char** argv) {
    try {
        int n = 1, fn = 0;
        bool help = false;

        std::pair<const char*, int> functions[] = {
            { "one", 1 }, { "two", 2 }
        };

        for (auto arg = argv+1; *arg; ) {
            bool ok =
                help << to::parse(arg, "-h", "--help") ||
                n    << to::parse<int>(arg, "-n", "--number") ||
                fn   << to::parse<int>(arg, to::keywords(functions), "-f", "--function");

            if (!ok) throw to::option_error("unrecognized argument", *arg);
        }

        if (help) {
            to::usage(argv[0], usage_str);
            return 0;
        }

        if (n<1) throw to::option_error("N must be at least 1");
        if (fn<1) throw to::option_error("Require FUNC");

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
