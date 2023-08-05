## HEAD

## 1.0 (2023-08-05)

* Slightly incompatible change: properly forward errors on exec,
  return 123 on any exit status 1-254 (like GNU xargs).
* Small bugfixes.

## 0.11 (2017-11-05)

* Add zsh completion.
* Fix `-F` to exit as soon as possible when a command failed.
* Fix when using `-s` and `-p` together.
* Better error handling for some rare situations.
* Made test suite more robust.

## 0.10 (2017-10-30)

* New flag `-p` to enable "percent rules" ala make(1), see the man
  page for details.
* New flag `-q` to run commands, hiding their output.
* Fix issues launching commands on FreeBSD and DragonFlyBSD.

## 0.9 (2017-07-17)

* New flag `-L` to enable line buffering on output.
* New flag `-LL` to prefix output lines with `$ITER`.
* `-j` now can scale with the available CPU cores
  (e.g. `-j0.5x` to use half the cores).
* `-vv` will trace `$ITER` and `exit status`.
* Fix issues related to having more children than we knew of.
* Documentation fixes and improvement by Larry Hynes.

## 0.8 (2017-06-02)

* Only read new arguments when we immediately need them

## 0.7 (2017-04-13)

* Error when the `-A` separator does not appear
* Bugfix: fix quoting of empty strings
* Documentation fixes
