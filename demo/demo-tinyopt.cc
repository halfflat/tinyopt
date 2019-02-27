#include <string>
#include <utility>
#include <vector>

#include <tinyopt/tinyopt.h>

// Parse command line options as:
//     -a, --apple N      Assign N to apple.
//     -b, --banana STR   Call banana() with string STR.
//     -c, --cranberry    Flag; calls cranberry() if present.
//     -d, --date VAR     Takes a keyword argument 'medjool' or 'piarom'.
//     -h, --help         Report usage.
//
// Other arguments are preserved and reported.

const char* usage_str =
    "[OPTION...] [word...]\n"
    "Options:\n"
    "  -a, --apple=N      Set apple value to N.\n"
    "  -b, --banana=STR   Call banana function with STR\n"
    "  -c, --cranberry    Call cranberry function\n"
    "  -d, --date=DATA    Set date to varienty 'medjool' or 'piarom'\n"
    "  -h, --help         Display usage information and exit\n";

enum class date_kind {
    none, medjool, piarom
};

std::pair<const char*, date_kind> date_tbl[] = {
    {"none"   , date_kind::none},
    {"medjool", date_kind::medjool},
    {"piarom",  date_kind::piarom}
};

void banana(const std::string& s) {
    std::cout << "banana(): " << s << '\n';
}

int cranberry() {
    std::cout << "cranberry()\n";
    return 1;
}

int main(int argc, char** argv) {
    int apple = 0;
    date_kind date = date_kind::none;
    std::vector<std::string> remaining;

    try {
        char** arg = argv+1;
        while (*arg) {
            if (apple << to::parse<int>(arg, 'a', "apple") ||
                banana << to::parse<std::string>(arg, 'b', "banana") ||
                cranberry << to::parse(arg, 'c', "cranberry") ||
                date << to::parse<date_kind>(arg, 'd', "date", to::keywords(date_tbl)))
            {
                continue;
            }

            if (to::parse(arg, 'h', "help")) {
                to::usage(argv[0], usage_str);
                return 0;
            }

            if (*arg[0]=='-') throw to::unrecognized_option(*arg);
            remaining.push_back(*arg++);
        }

        std::cout << "apple: " << apple << '\n';
        std::cout << "date: " << date_tbl[(int)date].first << '\n';
        std::cout << "remaining arguments:\n";
        for (auto& r: remaining) std::cout << "  " << r << '\n';
    }
    catch (to::option_error& e) {
        to::usage(argv[0], usage_str, e.what());
    }
}
