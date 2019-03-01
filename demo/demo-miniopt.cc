#include <fstream>
#include <utility>

#include <tinyopt/smolopt.h>

// Parse command line options as:
//   -a              Increment count of 'a'.
//   +a              Decrement count of 'a'
//   -b              Increment count of 'b'.
//   +b              Decrement count of 'b'.
//   -c, --cat NAME  Override name of cat.
//   -x N            Collect numbers N in vector x.
//   -w              Set flag w.
//   +w              Unset flag w.
//   --save          Save options in saved.opt
//   KEYWORD         KEYWORD is one of 'up' or 'down'
//
// Options are read first from saved.opt if present.
// Non-long format options can be combined, e.g.
//     -aaab +wba
// should be equivalent to -a -a -a -b +w +b +a.

enum updown { none = 0, up = 1, down = 2 };
std::pair<std::string, enum updown> updown_keywords[] = {
    {"up", up}, {"down", down}
};

int main(int argc, char** argv) {
    using namespace to::literals;
    to::saved_options restore;

    std::ifstream saved("saved.opt");
    if (saved) saved >> restore;
    saved.close();

    int a = 0, b = 0;
    std::string cat = "Charles";
    bool w = false;
    std::vector<int> x;
    bool save = false;
    enum updown keyword;

    to::option options[] = {
        { to::increment(a),     "-a"_compact, to::flag },
        { to::increment(a, -1), "+a"_compact, to::flag },
        { to::increment(b),     "-b"_compact, to::flag },
        { to::increment(b, -1), "+b"_compact, to::flag },
        { to::set(w),           "-w"_compact, to::flag },
        { to::set(w, false),    "+w"_compact, to::flag },
        { to::set(save),        "--save", to::flag, to::ephemeral },
        { cat,                  "-c"_compact, "--c" },
        { to::push_back(x),     "-x"_compact},
        { {keyword, to::keywords(updown_keywords)}, to::single }
    };

    auto allopts = to::run(options, argc, argv+1, restore);
    if (!allopts) return 0;

    std::cout << "unparsed arguments:";
    for (auto a = argv+1; *a; ++a) std::cout << " '" << *a << "'";
    std::cout << "\n\n";

    std::cout << "a: "   << a << "\n";
    std::cout << "b: "   << b << "\n";
    std::cout << "cat: " << cat << "\n";
    std::cout << "w: "   << std::boolalpha << w << "\n";
    std::cout << "x:";
    for (auto v: x) std::cout << " " << v;
    std::cout << "\n";
    std::cout << "keyword: ";
    switch (keyword) {
    case up:   std::cout << "up\n";  break;
    case down: std::cout << "down\n"; break;
    default:   std::cout << "none\n"; break;
    }

    if (save) {
        std::ofstream saved("saved.opt");
        saved << *allopts;
    }
}
