#!/bin/sh
export "PATH=.:$PATH"

printf '1..13\n'
printf '# error handling\n'

tap3 'exit code on success' <<'EOF'
xe
>>>= 0
EOF

tap3 'exit code on other error' <<'EOF'
true | xe -j NaN
>>>= 1
EOF

tap3 'exit code on when command fails with 1-125' <<'EOF'
xe -s 'exit 42'
<<<
a
>>>= 123
EOF

tap3 'exit code on when command fails with 255' <<'EOF'
xe -s 'exit 255'
<<<
a
>>>= 124
EOF

tap3 'exit code when process was killed' <<'EOF'
xe perl -e 'kill "KILL", $$'
<<<
a
>>>= 125
EOF

# possible false positive result when exec returns ENOENT instead of ENOTDIR here
tap3 'exit code when command cannot be run' <<'EOF'
xe /dev/null/calc.exe
<<<
a
>>>= 126
EOF

tap3 'exit code when command was not found' <<'EOF'
xe /bin/calc.exe
<<<
a
>>>= 127
EOF

tap3 'exit code on empty input when run with -R' <<'EOF'
xe -R echo a
>>>= 122
EOF

tap3 'doesn'\''t stop on errors by default' <<'EOF'
xe -s 'if [ b = $1 ]; then false; else echo $1; fi'
<<<
a
b
c
>>>
a
c
>>>= 123
EOF

tap3 'stops on first error with -F' <<'EOF'
xe -F -s 'if [ b = $1 ]; then false; else echo $1; fi'
<<<
a
b
c
>>>
a
>>>= 123
EOF

tap3 'should close stdin when arguments were read from it' <<'EOF'
xe -s 'sed q'
<<<
a
b
c
>>>
EOF

tap3 'should not close stdin when arguments were read from command line' <<'EOF'
yes | xe -a -s "sed q" -- 1 2 3
>>>
y
y
y
EOF

tap3 'should not close stdin when arguments were read from file' <<'EOF'
yes | xe -f NEWS.md -s 'sed q' 2>&1 | sed 3q
>>>
y
y
y
EOF
