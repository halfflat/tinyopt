#include <fstream>

#include <tinyopt/miniopt.h>

// Parse command line otpions as:
//   -a              Increment count of 'a'.
//   +a              Decrement count of 'a'
//   -b              Increment count of 'b'.
//   +b              Decrement count of 'b'.
//   -c, --cat NAME  Override name of cat.
//   -x N            Collect numbers N in vector x.
//   -w              Set flag w.
//   +w              Unset flag w.
//   --save          Save options in saved.opt
//
// Options are read first from saved.opt if present.
// Non-long format options can be combined, e.g.
//     -aaab +wba
// should be equivalent to -a -a -a -b +w +b +a.

int main(int argc, char** argv) {
    to::option_set restore;

    std::ifstream saved("saved.opt");
    if (saved) saved >> restore;

    int a = 0, b = 0;
    std::string cat = "Charles";
    bool w = false;
    std::vector<int> x;

    to::option options[] = {
        { to::increment(a), "-a"_compact },

    };

