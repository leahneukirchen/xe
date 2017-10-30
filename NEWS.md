## HEAD

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
* Bug: fix quoting of empty strings
* Documentation fixes
