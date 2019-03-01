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
iterates through the argument list itself. `smolopt` aims to answer
demands from another project where it was deemed that `tiyopt` was
in fact too tiny, and so does much more of the heavy lifting.

Design goals:

* Header only.
* C++14 and C++17 compatible.
* Fairly short and easy to customize.

Features:

* Support 'short' and 'long' style arguments: `-v 3` and `--value=3`.
* [`smolopt` only] Support 'compact' bunching of arguments: `-abc 3` vs `-a -b -c 3`.
* [`smolopt` only] Save and restore options and arguments in a shell-compatible format,
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

## Building

Tinyopt/smolopt is a header-only library, but the supplied
Makefile will build the unit tests and examples.

The Makefile is designed to support out-of-tree building, and the recommended
approach is to create a build directory, symbolicly link the project Makefile
into the directory, and build from there. For example, to check out, build the
tests and examples, and then run the unit tests:
```
    % git clone git@github.com:halfflat/tinyopt
    % cd tinyopt
    % mkdir build
    % cd build
    % ln -s ../Makefile .
    % make
    % ./unit
```

## Simple examples

More examples are found in the `ex/` subdirectory.

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
    "  -f, --function=FUNC  Perform FUNC, which is one of: one, two\n"
    "  -h, --help           Display usage information and exit\n";

int main(int argc, char** argv) {
    try {
	int n = 0, fn = 0;
	bool help = false;

	std::pair<const char*, int> functions[] = {
	    { "one", 1 }, { "two", 2 }
	};

	for (auto arg = argv+1; *arg; ) {
            bool ok =
	        help << to::parse(arg, 'h', "help") ||
	        n    << to::parse<int>(arg, 'n', "number") ||
	        fn   << to::parse<int>(arg, 'f', "function", to::keywords(functions));

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
	to::usage(argv[0], usage_str, e.what());
	return 1;
    }
}
```

Equivalent `smolopt` code.
```
#include <string>
#include <utility>
#include <tinyopt/smolopt.h>

const char* usage_str =
    "[OPTION]...\n"
    "\n"
    "  -n, --number=N       Specify N\n"
    "  -f, --function=FUNC  Perform FUNC, which is one of: one, two\n"
    "  -h, --help           Display usage information and exit\n";

int main(int argc, char** argv) {
    try {
	int n = 0, fn = 0;

	std::pair<const char*, int> functions[] = {
	    { "one", 1 }, { "two", 2 }
	};

        auto help = [argv0 = argv[0]] { to::usage(argv0, usage_str); };

	to::option opts[] = {
	    { n, "-n", "--number" },
	    { {fn, to::keywords(functions)}, "-f", "--function", to::mandatory },
	    { to::action(help), to::flag, to::exit, "-h", "--help" }
	};

	if (!to::run(opts, argc, argv)) return 0;

	if (argv[1]) throw to::option_error("unrecogonized argument", argv[1]);
	if (n<1) throw to::option_error("N must be at least 1");

	// Do things with arguments:

	for (int i = 0; i<n; ++i) {
	    std::cout << "Performing function #" << fn << "\n";
	}
    }
    catch (to::option_error& e) {
	to::usage(argv[0], usage_str, e.what());
	return 1;
    }
}
```

## Documentation

All tinyopt and miniopt code lives in the namespace `to`. This namespace
is omitted in the descriptions below.

### Common classes and helpers:

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

#### `option_error`

An exception class derived from `std::runtime_error`. It has two constructors:
* `option_error(const std::string&)` simply sets the what string to the argument.
* `option_error(const std::string& message, const std::string& arg)` sets the what string to
   the value of `arg+": "+mesage`.

The option parsers can throw exceptions derived from `option_error`, namely:
`option_parse_error`, `missing_mandatory_option`, and `missing_argument`.

#### `usage(const char *argv0, const std::string& usagemsg)`

Extract a program name from `argv0` (everything after the last '/' if present) and
print a message to standard out in the form "Usage: <program-name> <usagemsg>\n".

#### `usage(const char *argv0, const std::string& usagemsg, const srd::string& error)`

Extract a program name from `argv0` (everything after the last '/' if present) and
print a message to standard error in the form
"<program-name>: <error>\nUsage: <program-name> <usagemsg>\n".

### Parsers

A parser is a function or functional object with signature `maybe<X> (const char*)`
for some type `X`. They are used to try to convert a C-string argument into a value.

If no explicit parser is given to a tinyopt `parse` function or to a smolopt `option`,
the default parser `default_parser` is used, which will use `std::istream::operator>>`
to read the supplied argument.

Tinyopt supplies additional parsers:

* `keyword_parser<V>`

   Constructed from a table of key-value pairs, the `keyword_parser<V>` parser
   will return the first value found in the table with matching key, or `nothing`
   if there is no match.

   The `keywords(pairs)` function constructs a `keyword_parser<V>` object from the
   collection of keyword pairs `pairs`, where each element in the collection is
   a `std::pair` or `std::tuple`. The first component of each pair is used
   to construct the `std::string` key in the keyword table, and the second the
   value. The value type `V` is deduced from this second component.

* `delimited_parser<P>`

   The delimited parser uses another parser of type `P` to parse individual
   elements in a delimited sequence, and returns a `std::vector` of the
   corresponding values.

   The convenience constructor `delimited<V>(char delim = ',')` will make
   a `delimited_parser` using the default parser for `V` and delimiter
   `delim` (by default, a comma).

   `delimited(char delim, P&& parser)` is a convenience wrapper for
   `delimited_parser<P>::delimited_parser(delim, parser)`.

### Tinyopt

Tinyopt makes some strong assumptions about valid option names:
* Each option can have at most one short name of the form "-x" and at most one long name of the form "--long".
* Short options take their parameter from the following argument; long options take theirs from the next
  argument, or from the same argument if given with an equal sign.
  For example, for an option with short name 'a' and long name "apple", all of the following are equivalent ways to specify the argument:
  `-a 3`, `--apple=3`, `--apple 3`.

Tinyopt provides one (overloaded) function for option parsing, `parse`.
* `maybe<void> parse(char**& argp, char shortopt, const char* longopt = nullptr)`

   Attempt to parse an option with no argument (i.e. a flag) at `argp`, given by
   the short option `shortopt` (if not NUL) or the long option `longopt` (if not
   a null pointer). Returns an empty `maybe<void>` value if it fails to match
   the option.

   If the match is successful, increment `argp` to point to the next argument.

* `maybe<V> parse(char**& argp, char shortopt, const char* longopt = nullptr, constr P& parser = P{})`

   Attempt to parse an option with an argument to be interpreted as type `V`,
   using the short and long option names as above. If no `parser` is supplied,
   the default parser for `V` is used. 

The `parse` functions will throw `missing_argument` if no argument is found
for a non-flag option, and `option_parse_error` if the parser for an argument
returns `nothing`.


### Smolopt

TODO.

