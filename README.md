## xe: simple xargs and apply replacement

`xe` is a new tool for constructing command lines from file listings
or arguments, which includes the best features of `xargs(1)` and
`apply(1)`.

`xe` means "execute for every ...".

## Benefits

Over xargs:
* Sane defaults (behaves like `xargs -d'\n' -I{} -n1 -r`).
* No weird parsing, arguments are separated linewise or by NUL byte.
* Can also take arguments from command-line.
* No shell involved unless `-s` is used.
* `{}` replacing possible with multiple arguments.
* Support for patterns to run different commands depending on the argument.

Over apply:
* Parallel mode.
* Sane argument splitting.
* Can use shell-syntax instead of escape characters.

## [Man page](README)

## Installation

Use `make all` to build, `make install` to install relative to `PREFIX`
(`/usr/local` by default).  The `DESTDIR` convention is respected.
You can also just copy the binary into your `PATH`.

## Copyright

xe is in the public domain.

To the extent possible under law,
Leah Neukirchen <leah@vuxu.org>
has waived all copyright and related or
neighboring rights to this work.

http://creativecommons.org/publicdomain/zero/1.0/
