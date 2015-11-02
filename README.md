## xe: simple xargs and apply replacement

`xe` is a new tool for constructing command lines from file listings
or arguments, which includes the best features of `xargs(1)` and
`apply(1)`.

`xe` means "execute for every ...".

## Benefits

Over xargs:
* Sane defaults (behaves like `xargs -d'\n' -I{} -L1 -n1 -r`).
* No weird parsing, arguments are seperated linewise or by NUL byte.
* Can also take arguments from command-line.
* No shell involved unless `-s` is used.

Over apply:
* Parallel mode.
* Sane argument splitting.
* Can use shell-syntax instead of escape characters.

## Usage:

	xe [-0Rknv] [-I arg] [-N maxargs] [-j maxjobs] COMMAND...
	   | -s SHELLSCRIPT
	   | -a COMMAND... -- ARGS...
	   | -A ARGSEP COMMAND... ARGSEP ARGS...

* `-0`: input filenames are seperated by NUL bytes (default: newlines).
* `-R`: return with status 122 when no arguments have been passed.
* `-k`: keep going: don't stop when a command failed to execute.
* `-n`: don't run the commands, just print them.
* `-v`: print commands before running them.
* `-I`: replace occurences of *arg* with the argument (default: `{}`).
* `-N`: pass upto *maxargs* arguments to each COMMAND (default: 1).
* `-j`: run up to *maxjobs* processes concurrently.
* `COMMAND...`: default operation: each command line argument is
  passed as-is, `{}` is replaced by the argument (not with `-N` > 1).
* `-s SHELLSCRIPT`: The argument `SHELLSCRIPT` is evaluated using `/bin/sh`
  with the arguments (up to `-N`) passed as `$1`, `$2`, `$3`...
  (this behaves as if `/bin/sh -c SHELLSCRIPT -` is passed as plain COMMAND).
* `-a`: take arguments from commandline, starting after the first `--`.
* `-A`: take arguments from commandline, starting after the first *argsep*.

If no argument is passed, default to "printf %s\n".

The current iteration is passed as `$ITER` to the child process
(increased on every exec()).

## Return code

Like GNU and OpenBSD xargs:

* 0 on success
* 123 if any invocation of the command exited with status 1-125
* 124 if the command exited with status 255
* 125 if the command is killed by a signal
* 126 if the command cannot be run
* 127 if the command is not found
* 1 if some other error occurred.

Additionally, 122 is returned when `-R` is passed and no
input/arguments were passed.

## Installation

Use `make all` to build, `make install` to install relative to `PREFIX`
(`/usr/local` by default).  The `DESTDIR` convention is respected.
You can also just copy the binary into your `PATH`.

## Copyright

xe is in the public domain.

To the extent possible under law,
Christian Neukirchen <chneukirchen@gmail.com>
has waived all copyright and related or
neighboring rights to this work.

http://creativecommons.org/publicdomain/zero/1.0/
