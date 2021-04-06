## Version 1.0

### Bug fixes
* Correct parsing of arguments with the default parser relied upon nonstandard
  ios flag behaviour.

### Other changes
* Removed the distinction between the `tinyopt` and `smolopt` interfaces.
  Options can be parsed one-by-one with `to::parse`, or in a batch with
  `to::run`.

* `to::parse` can take any number of short- or long-style keys, instead
  of a single short and single long argument.

* The hard-coded `--` option, indicating that remaining arguments should
  be left unprocessed, has been removed. Any flag can be given this
  behabiour with the new `to::stop` flag.

* All functionality is now in a single header, `tinyopt/tinyopt.h`.

* CI unit testing is now performed on pushes and pull request on GitHub.
