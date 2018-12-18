Smaller, better? option parsing.

## Goals

There are 1391 command line option parsing libraries for C++, but only a few
are popular. [TCLAP](http://tclap.sourceforge.org) is one, but it is heavy
weight, and has annoying default behaviour which is hard to customize.
There's also [Boost.Program_options](https://www.boost.org/doc/libs/release/libs/program_options/)
which is also kinda huge.

So this project constitutes yet another header-only option parsing library.
Actually, two. `tinyopt` is quite minimal: all it does is handle the
problem of matching and parsing an option specification; user code
iterates through the argument list itself. `miniopt` aims to answer
demands from another project where it was deemed that `tiyopt` was
too tiny, and does much more of the heavy lifting.

Design goals:

* Header only.
* C++14 and C++17 compatible.
* Fairly short and easy to customize.

Features:

* Support 'short' and 'long' style arguments: `-v 3` and `--value=3`.
* [`miniopt` only] Support 'compact' bunching of arguments: `-abc 3` vs `-a -b -c 3`.
* [`miniopt` only] Save and restore options and arguments in a shell-compatible format,
  allowing e.g. `program `cat previous-options` --foo=bar`.

Non-features:

* Doesn't support multibyte encodings with shift sequences.
  This is due to laziness.
* (But does try not to break UTF-8 at least.)
  This, essentially, is also due to laziness.
* Does not automatically generate help/usage text.
  The user code really ought to know best in this circumstance.
* No localization in `usage` helper function.
  This would not be terribly hard to add, but will maybe sorta add this later.
* Does not automatically enforce any inter-option constraints.
  This also, can require arbitrarily complex support in any library,
  where instead it can be handled much more easily user side.

## Simple examples

`tinyopt` code for parsing options three options, one numeric,
one a keyword from a table, and one just a flag.

```
#include <string>
#include <utility>
#include <tinyopt/tinyopt.h>

const char* usage_str =
    "[OPTION]...\n"
    "\n"
    "  -n, --number=N       Specify N\n"
    "  -a, --action=ACTION  Do ACTION, which is one of: one, two\n"
    "  -h, --help           Display usage information and exit\n";

int main(int argc, char** argv) {
    using to::parse_opt;
    try {
	int n = 0, action = 0;
	bool help = false;

	std::pair<std::string, int> actions[] = {
	    { "one", 1 }, { "two", 2}
	};

	for (auto arg = argv+1; *arg; ) {
	    help   << parse_opt<>(arg, 'h', "help") ||
	    n      << parse_opt<int>(arg, 'n', "number") ||
	    action << parse_opt<int>(arg, 'a', "action", keywords(actions)) ||
	    throw to::parse_opt_error(arg, "unrecognized argument");
	}

	if (n<1) throw to::parse_opt_error(arg, "N must be at least 1");
	if (action<1) throw to::parse_opt_error(arg, "Require ACTION");
	if (help) {
	    to::usage(argv[0], usage_str);
	    return 0;
	}

	// Do things with arguments:

	for (int i = 0; i<n; ++i) {
	    std::cout << "Doing action #" << action << "\n";
	}
    }
    catch (to::parse_opt_error& e) {
	to::usage(argv[0], usage_str, e.what());
	return 1;
    }
}
```

Equivalent `miniopt` code.
```
#include <string>
#include <utility>
#include <tinyopt/tinyopt.h>

const char* usage_str =
    "[OPTION]...\n"
    "\n"
    "  -n, --number=N       Specify N\n"
    "  -a, --action=ACTION  Do ACTION, which is one of: one, two\n"
    "  -h, --help           Display usage information and exit\n";

int main(int argc, char** argv) {
    try {
	int n = 0, action = 0;
	bool help = false;

	std::pair<std::string, int> actions[] = {
	    { "one", 1 }, { "two", 2}
	};

	to::option opts[] = {
	    { n, "-n", "--number" },
	    { action, keywords(actions), "-a", "--action", to::mandatory },
	    { help, to::flag, "-h", "--help" }
	};

	to::run(opts, argc, argv);

	if (argv[1]) throw to::parse_opt_error(argv[1], "unrecogonized argument");
	if (n<1) throw to::parse_opt_error(arg, "N must be at least 1");
	if (help) {
	    to::usage(argv[0], usage_str);
	    return 0;
	}

	// Do things with arguments:

	for (int i = 0; i<n; ++i) {
	    std::cout << "Doing action #" << action << "\n";
	}
    }
    catch (to::parse_opt_error& e) {
	to::usage(argv[0], usage_str, e.what());
	return 1;
    }
}
```

## Documentation

All tinyopt and miniopt code lives in the namespace `to`. This namespace
is omitted in the descriptions below.

Common classes and helpers:

#### `template <typename V> struct maybe`

`maybe<V>` is a simple work-alike for (some of) the C++17 `std::optional<V>` class.
A default constructed `maybe<V>` has no value, and evaluates to false in a `bool`
context; it will otherwise evaluate to true.

If `m` is an object of type `maybe<V>`, then `*m` evaluates to the contained value
if defined. `m.value()` does the same, but will throw an exception of type
`std::invalid_argument` if `m` does not contain a value.

The special value `nothing` is implicitly convertible to an empty `maybe<V>` for any `V`.
The expression `just(v)` function returns a `maybe>` holding the value `v`.

As a special case, `maybe<void>` simply maintains a has-value state; it will return
true in a `bool` context if has been initialized or assigned with any `maybe<V>`
that contains a value, or by any other value that is not `nothing`. `something`
is a pre-defined non-empty `maybe<void>`.

`maybe<V>` values support basic monadic-like functionality via `operator<<`.
* If `x` is an lvalue and `m` is of type `maybe<U>`, then 
  `x << m` has type `maybe<V>` (`V` is the type of `x=*m`) and assigns `m.value()` to `x`
  if `m` has a value.
* If `f` is a function or function object with signature `V f(U)`, and `m` is of type `maybe<U>`, then 
  `f << m` has type `maybe<V>` and contains `f(*m)` if `m` has a value.
* And similarly for `maybe<void>` and functions `V f()`.

#### `parse_opt_error`

An exception class derived from `std::runtime_error`. It has two constructors:
* `parse_opt_error(const std::string&)` simply sets the what string to the argument.
* `parse_opt_error(const char* arg, const std::string& s)` sets the what string to
  the value of `arg+":"+s`.

#### `usage(const char *argv0, const std::string& usagemsg)`

Extract a program name from `argv0` (everything after the last '/' if present) and
print a message to standard out in the form "Usage: <program-name> <usagemsg>\n".

#### `usage(const char *argv0, const std::string& usagemsg, const srd::string& error)`

Extract a program name from `argv0` (everything after the last '/' if present) and
print a message to standard error in the form
"<program-name>: <error>\nUsage: <program-name> <usagemsg>\n".


### Tinyopt

Tinyopt makes some strong assumptions about valid option names:
* Each option can have at most one short name of the form "-x" and at most one long name of the form "--long".
* Short options take their parameter from the following argument; long options take theirs from the next
  argument, or from the same argument if given with an equal sign.
  For example, for an option with short name 'a' and long name "apple", all of the following are equivalent ways to specify the argument:
  `-a 3`, `--apple=3`, `--apple 3`.

### Miniopt
