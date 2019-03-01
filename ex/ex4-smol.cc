#include <iostream>
#include <fstream>
#include <string>
#include <utility>

#include <tinyopt/smolopt.h>

const char* usage_str =
    "[OPTION]...\n"
    "\n"
    "  -a           increment 'a' count\n"
    "  -b           increment 'b' count\n"
    "  -c           increment 'c' count\n"
    "  set/one      set flag 'one'\n"
    "  set/two      set flag 'two'\n"
    "  set/three    set flag 'three'\n"
    "  --save=FILE  save command line options in FILE\n"
    "  -h, --help   display usage information and exit\n";

int main(int argc, char** argv) {
    try {
        using namespace to::literals;
        auto help = [argv0 = argv[0]] { to::usage(argv0, usage_str); };

        int a = 0, b = 0, c = 0;
        bool one = false, two = false, three = false;
        std::string save_file;

	to::option opts[] = {
	    { to::increment(a), to::flag, "-a"_compact },
	    { to::increment(b), to::flag, "-b"_compact },
	    { to::increment(c), to::flag, "-c"_compact },
	    { to::set(one),     to::flag, "set/one"_compact },
	    { to::set(two),     to::flag, "set/two"_compact },
	    { to::set(three),   to::flag, "set/three"_compact },
	    { save_file,        to::ephemeral, "--save" },
	    { to::action(help), to::flag, to::exit, "-h", "--help" }
	};

	auto saved = to::run(opts, argc, argv);
        if (!saved) return 0;

        if (!save_file.empty()) {
            std::ofstream f(save_file);
            f << *saved << '\n';
        }

        if (a>0) std::cout << "a=" << a << '\n';
        if (b>0) std::cout << "b=" << b << '\n';
        if (c>0) std::cout << "c=" << c << '\n';
        if (one) std::cout << "set/one\n";
        if (two) std::cout << "set/two\n";
        if (three) std::cout << "set/three\n";
    }
    catch (to::option_error& e) {
	to::usage(argv[0], usage_str, e.what());
	return 1;
    }
}
