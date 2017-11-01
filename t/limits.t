#!/bin/sh
export "PATH=.:$PATH"

printf '1..2\n'
printf '# limit checks, expecting maximal POSIX limits available\n'

tap3 'argscap check' <<'EOF'
dd if=/dev/zero bs=1 count=17711 2>/dev/null |
	tr "\0" "\012" |
	xe -N0 -s 'echo $#'
>>>
8187
8187
1337
EOF

tap3 'argslen check' <<'EOF'
perl -e 'print "x"x8000, "\n" for 1..42' |
	xe -N0 -s 'echo $#'
>>>
16
16
10
EOF
