#!/bin/sh
export "PATH=.:$PATH"

printf '1..4\n'
printf '# slow tests\n'

tap3 'is eager' <<'EOF'
{ { echo 1; sleep 1; echo 11 >/dev/stderr; echo 2; } | xe echo; } 2>&1
>>>
1
11
2
EOF

tap3 'using -L' <<'EOF'
{ echo 1; sleep 1;
  echo 2; sleep 1;
  echo 3; } |
	xe -j2 -L -s 'printf $1; sleep 1; echo $1'
>>>
11
22
33
EOF

tap3 'using -LL' <<'EOF'
{ echo 1; sleep 1;
  echo 2; sleep 1;
  echo 3; } |
	xe -j2 -LL -s 'printf $1; sleep 1; echo $1'
>>>
0001= 11
0002= 22
0003= 33
EOF

tap3 'using -vvL' <<'EOF'
{ echo 1; sleep 1;
  echo 2; sleep 1;
  echo 3; } |
	xe -j2 -vvL -s 'printf %s $1; sleep 1; echo $1' | wc -l
>>> /\s*9$/
EOF
