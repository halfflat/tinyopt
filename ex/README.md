# Tinyopt examples

## Example 1 — basic demo

This example incorporates three options/flags:

1. The `-n`/`--number` option sets an integer `n`.

2. The `-f`/`--function` option sets an integer `fn` according to a keyword: 'one' or 'two'.
   This option is mandatory.

3. The `-h`/`--help` flag causes the program to print help information and exit.

Both implementations, `ex1-parse` and `ex1-run` use `to::usage()` to handle
option-related errors and to print help information.

The `to::parse` implementation, `ex1-parse` handles the `--help` option via
a boolean flag, and checks explicitly that the `--function` option is provided.
In contrast, the `to::run` version uses an _action_ to handle the help function,
and the `--function` option is given the `to::mandatory` flag.

Note the use of `to::exit` in the `--help` option in `ex1-run`: this
stops further option processing, and causes `to::run` to return an
empty `maybe` value.

## Example 2 — delimited values

This demonstrates the use of the delimited parser to parse a comma-separated
list of integers. The numbers are given as the argument to the option `--sum`.

## Example 3 — remaining arguments

As the `to::parse` interface requires the code to go through each
command line argument in turn, non-option arguments can be collated
manually. When using `to::run`, however, the argument list will
be modified in place, removing matched options.

The two approaches are exemplified in `ex3-parse` and `ex3-run`.

## Example 4 — compact options

Compact or combined options are not supported with `to::parse`,
but are available through the use of compact keys with `to::run`.

This example counts the occurances of flags `-a`, `-b`
and `-c`, and reports if any of the options `set/one`, `set/two`,
or `set/three` are set. Any compact keys which share a common prefix can be combined
in a single command line argument, provided that none but the last take
any values.

As an example, the options `set/one` `set/two` and `set/three` can be combined as:
```
% ex4-run set/one/two/three
set/one
set/two
set/three
```

This example also shows how the saved option data can be used to
replay previous command line invocations, e.g.
```
% ex4-run --save saved.opt -aab
a=2
b=1
# cat saved.opt
-a -a -b
% ex4-run -c $(<saved.opt)
a=2
b=1
c=1
```
 
## Example 5 — modal options

Specifying `to::when` and `to::then` in an option allows for
modal behaviour when parsing the command line arguments.

In `ex5-run`, the flag "echo" on the command line causes the
program to emit the following words one per line, while "ohce",
in turn, causes the following words to be emitted backwards.

The example code also demonstrates the use of `to:error`, an action helper
that will cause a `to::user_option_error` to be thrown.

Sample run:
```
% ex5-run hello
ex5-run: unrecognized keyword
Usage: ex5-run [echo [word...] | ohce [word...]] ...

Print words afer 'echo' one per line;
Print words after 'oche' backwards, one per line.
% ex5-run echo how now ohce brown echo cow
how
now
nworb
cow
```
```
