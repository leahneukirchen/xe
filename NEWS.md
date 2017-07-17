## HEAD

## 0.9 (2017-07-17)

* New flag `-L` to enables line buffering on output.
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
