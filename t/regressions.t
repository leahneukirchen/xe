#!/bin/sh
export "PATH=.:$PATH"

printf '1..1\n'
printf '# regressions\n'

tap3 '0fb64a4 quoting of empty strings' <<'EOF'
xe -N2 -v true
<<<
foo

>>>2
true foo ''
EOF
