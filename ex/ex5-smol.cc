#include <iostream>
#include <string>

#include <tinyopt/smolopt.h>

const char* usage_str =
    "[echo [word...] | ohce [word...]] ...\n"
    "\n"
    "Print words afer 'echo' one per line;\n"
    "Print words after 'oche' backwards, one per line.";

void echo(const std::string& s) {
    std::cout << s << "\n";
}

void ohce(const std::string& s) {
    echo(std::string(s.rbegin(), s.rend()));
}

int main(int argc, char** argv) {
    try {
	to::option opts[] = {
            // 'echo' and 'ohce' are always recognized:
            { {}, to::flag, "echo", to::then(1) },
            { {}, to::flag, "ohce", to::then(2) },

            // If mode is zero and we see a word, it's an error.
	    { to::error("unrecognized keyword"), to::when(0) },

            // If mode is one, echo argument.
	    { to::action(echo), to::when(1) },

            // If mode is two, echo argument in reverse.
	    { to::action(ohce), to::when(2) }
	};

	to::run(opts, argc, argv+1);
    }
    catch (to::option_error& e) {
	to::usage(argv[0], usage_str, e.what());
	return 1;
    }
}
