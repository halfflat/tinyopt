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
demands from another project where it was deemed that `tinyopt` was
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
  This can require arbitrarily complex support in any library,
  where it can instead be handled much more easily on the user side.

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
	int n = 1, fn = 0;
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

### Common classes and helpers

#### `template <typename V> struct maybe`

`maybe<V>` is a simple work-alike for (some of) the C++17 `std::optional<V>` class.
A default constructed `maybe<V>` has no value, and evaluates to false in a `bool`
context; it will otherwise evaluate to true.

If `m` is an object of type `maybe<V>`, then `*m` evaluates to the contained value
if defined. `m.value()` does the same, but will throw an exception of type
`std::invalid_argument` if `m` does not contain a value.

The special value `nothing` is implicitly convertible to an empty `maybe<V>` for any `V`.
The expression `just(v)` function returns a `maybe<V>` holding the value `v`.

As a special case, `maybe<void>` simply maintains a has-value state; it will return
true in a `bool` context if has been initialized or assigned with any `maybe<V>`
that contains a value, or by any other value that is not `nothing`. `something`
is a pre-defined non-empty `maybe<void>`.

`maybe<V>` values support basic monadic-like functionality via `operator<<`.
* If `x` is an lvalue and `m` is of type `maybe<U>`, then 
  `x << m` has type `maybe<V>` (`V` is the type of `x=*m`) and assigns `m.value()` to `x`
  if `m` has a value. In the case that `U` is `void`, then the value of `m` is taken
  to be `true`.
* If `f` is a function or function object with signature `V f(U)`, and `m` is of type `maybe<U>`, then 
  `f << m` has type `maybe<V>` and contains `f(*m)` if `m` has a value.
* if `f` has signature `V f()`, and `m` is of type `maybe<U>` or `maybe<void>`, then
  `f << m` has type `maybe<V>` and contains `f()` if `m` has a value.

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

Smolopt hands more control to the library for option parsing. The basic
workflow is:

1. The user code sets up a collection of `option` objects, each of which describe
   a command line option or flag, and how to handle the result of parsing it.
2. This collection is passed to the `run` function, along with `argc` and `argv`.
   `argv` is modified in place to remove matched options; anything remaining
   can be handled by the user code.
3. The `run` function returns a saved set of matched options that can, for example,
   be saved in program output for tracking processing steps, or in some file
   to allow for re-execution of the code with the same arguments.

An `option` describes one command line flag or option with an argument. It has
three components: a `sink`, that describes what to do with a successfully parsed
option; a set of `key`s, which are how the option is presented on the command line;
and a set of option flags that modify behaviour.

#### Sinks

A sink wraps a function that takes a `const char*` value, representing the
argument to an option, and returns a `bool`. A return value of `true` indicates
a successful parsing of the argument; `false` represents a parse error.

A sink has three constructors:
* `sink(sink::action_t, Action a)`

  `action_t` is a tag type, indicating that the second argument is a functional
  to be used directly as the wrapped function. `sink::action` is a value with
  this for use in this constructor.

  This constructor is used by the `action` wrapper function described below.

* `sink(V& var, P parser)`

  Make a sink that uses the parser `P` (see above for a description of what
  constitutes a tinyopt parser) to get a value of type `V`, and write that
  value to `var`.

* `sink(V& var)`

  Equivalent to `sink(var, default_parser<V>{})`

If an option doesn't have any associated argument, i.e. it is a flag,
the `sink` object is passed `nullptr`.

The `action` wrapper function takes a nullary or unary function or functional
object, and optionally a parser for the function's argument. It returns a
`sink` object that applies the default or supplied parser object
and if successful, calls the function with the parsed value.

#### Sink adaptors

The library supplies some convenience adaptors for making `sink`s for common
situations:

* `push_back(Container& c, P parser = P{})`

  Append parsed values to the container `c` using `Container::push_back`.
  The default parser is `default_parser<Container::value_type>`.

* `set(V& v, X value)`

   Set `v` to `value`, ignoring any argument.

* `set(V& v)`

   Set `v` to `true`, ignoring any argument.

* `increment(V& v, X delta)`

   Perform `v += delta`, ignoring any argument.

* `increment(V& v)`

   Perform `++v`, ignoring any argument.

#### Keys

Keys are how options are specified on the command line. They consist of
a string label and a style, which is one of `key::shortfmt`,
`key::longfmt`, or `key::compact`.

All options that take an argument will take that argument from the
next item in the argument list, and only options with a 'compact'
key can be combined together in a single argument.

An option with a 'long' key can additionally take its argument by
following the key with an equals sign and then the argument value.

As an example, let "-s" be a short option key, "--long" a long option key,
"-a" a compact option key for a flag, and "-b" a compact option key for
an option that takes a value. Then the follwing are equivalent ways
for specifying these options on a command line:

```
-s 1 --long 2 -a -b 3
```

```
-s 1 --long=2 -a -b3
```

```
-s 1 --long=2 -ab3
```

Keys can be constructed explicitly, implicitly from labels, or
from the use of string literal functions:

* `key(std::string label, enum key::style style)`, `key(const char* label, enum key::style style)`

   Make a key with given label and style.

* `key(std::string label)`, `key(const char* label)`

   Make a key with the given label. The style will be `key::shortfmt`, unless
   the label starts with a double dash "--". This constructor is implicit.

* `operator""_short`, `operator""_long`, `operator""_compact`.

   Make a key in the corresonding style from a string literal.

The string literal operators are included in an inline namespace `literals`
that can be included in user code via `using namespace to::literals`.

#### Flags

Option behaviour can be modified by supplying `enum option_flag` values:

* `flag` — Option takes no argument, i.e. it is a flag.
* `ephemeral` — Do not record this option in the saved data returned by `run()`.
* `single` — This option will be checked at most once, and then ignored for the remainder of option processing.
* `mandatory` — Throw an exception if this option does not appear in the command line arguments.
* `exit` — On successful parsing of this option, stop any further option processing and return `nothing` from `run()`.

These enum values are all powers of two and can be combined via bitwise or `|`.

#### Specifying an option

The `option` constructor takes a `sink` as the first argument, followed by
any number of keys and flags in any order.

An `option` may have no keys at all — these will always match an item in the
command line argument list, and that item will be passed directly to the
option's sink.

Some example specifications:
```
    // Saves integer argument to variable 'a':
    int a = 0;
    to::option opt_a = { a, "-a" };

    // Flag '-v' or '--verbose' that increases verbosity level, but is not
    // kept in the returned list of saved options.
    int verbosity = 0;
    to::option opt_v = { to::increment(verbosity), "-v", "--verbose", to::flag, to::ephemeral };

    // Save vector of values from one argument of comma separated values, e.g.
    // -x 1,2,3,4,5:
    std::vector<int> xs;
    to::option opt_x = { {xs, to::delimited(xs)}, "-x" };

    // Save vector of values one by one, e.g.,
    // -k 1 -k 2 -k 3 -k 4 -k 5
    to::option opt_k = { to::push_back{xs}, "-k" };

    // A 'help' flag that calls a help() function and stops fruther option processing.
    to::option opt_h = { to::action(help), to::flag, to::exit, "-h", "--help" };

    // Compact option keys using to::literals:
    using namespace to::literals;
    bool a = false, b = false, c = false;
    to::option flags[] = {
	{ to::set(a), "-a"_compact, to::flag },
	{ to::set(b), "-b"_compact, to::flag },
	{ to::set(c), "-c"_compact, to::flag }
    };
```

#### Saved options

The `to::run()` function (see below) returns `maybe<saved_options>`. The `saved_options`
object holds a record of the successfully parsed options. It wraps a `std::vector<std::string>`
that has one element per option key and argument, in the order they were matched
(excluding options with the `to::ephemeral` flag).

While the contents can be inspected via methods `begin()`, `end()`, `empty()` and `size()`,
it is primarily intended to support serialization. Overloads of `operator<<` and `operator>>`
will write and read a `saved_options` object to a `std::ostream` or `std::istream` object
respectively. The serialized format uses POSIX shell-compatible escaping with backslashes
and single quotes so that they be incorporated directly on the command line in later
program invocations.

#### Running a set of options

A command line argument list or `saved_options` object is run against a collection of `option`
specifications with `run()`. There are five overloads, each of which returns a `saved_options`
value in normal execution or `nothing` if an option with the `to::exit` flag is matched.

In the following `Options` is any iterable collection of `option` values.

* `maybe<saved_options> run(const Options& options, int& argc, char** argv)`

   Parse the items in `argv` against the options provided in the first argument.
   Starting at the beginning of the `argv` list, options with keys are checked first,
   in the order they appear in `options`, followed by options without keys.
   An item `--` in `argv` stops any further processing of options.

   Successfully parsed options are removed from the `argv` list in-place, and
   `argc` is adjusted accordingly.

* `maybe<saved_options> run(const Options& options, int& argc, char** argv, const saved_options& restore)`

   As for `run(options, argc, argv)`, but first run the options against the saved command line
   arguments in `restore`, and then again against `argv`.

   A mandatory option can be satisfied by the restore set or by the argv set.

* `maybe<saved_options> run(const Options& options, const saved_options& restore)`

   As for `run(options, argc, argv, restore)`, but with an empty argc/argv list.

* `maybe<saved_options> run(const Options& options, char** argv)`

   As for `run(options, argc, argv)`, but ignoring argc.

* `maybe<saved_options> run(const Options& options, char** argv, const saved_options& restore)`

   As for `run(options, argc, argv, restore)`, but ignoring argc.

Like the tinyopt `parse` functions, the `run()` function can throw `missing_argument` or
`option_parse_error`. In addition, it will throw `missing_mandatory_option` if an option
marked with `to::mandatory` is not found during command line argument parsing.

Note that the arguments in `argv` are checked from the beginning; when calling `run` from within,
e.g the main function `int main(int argc, char** argv)`, one should pass `argv+1` to `run`
so as to avoid including the program name in `argv[0]`.
