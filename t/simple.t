#!/bin/sh
export "PATH=.:$PATH"

printf '1..27\n'
printf '# simple tests\n'

tap3 'single argument run' <<'EOF'
xe echo
<<<
1
2
3
>>>
1
2
3
EOF

tap3 'dual argument run' <<'EOF'
xe -N2 echo
<<<
1
2
3
4
5
>>>
1 2
3 4
5
EOF

tap3 'unlimited argument run' <<'EOF'
xe -N0 echo
<<<
1
2
3
4
5
>>>
1 2 3 4 5
EOF

tap3 'empty input run' <<'EOF'
true | xe echo a
>>>
EOF

tap3 'dry run' <<'EOF'
xe -n echo x
<<<
a
b
c
>>>2
echo x a
echo x b
echo x c
EOF

tap3 'dry run quoting' <<'EOF'
xe -n echo x
<<<
a
b b
c
>>>2
echo x a
echo x 'b b'
echo x c
EOF

tap3 'verbose run' <<'EOF'
xe -v echo x
<<<
a
b
c
>>>
x a
x b
x c
>>>2
echo x a
echo x b
echo x c
EOF

tap3 'with no command' <<'EOF'
xe -N2
<<<
1
2
3
>>>
1
2
3
EOF

tap3 'using {}' <<'EOF'
xe echo a {} x
<<<
1
2
3
>>>
a 1 x
a 2 x
a 3 x
EOF

tap3 'using {} twice' <<'EOF'
xe echo {} x {}
<<<
1
2
3
>>>
1 x {}
2 x {}
3 x {}
EOF

tap3 'using -I%' <<'EOF'
xe -I% echo {} x %
<<<
1
2
3
>>>
{} x 1
{} x 2
{} x 3
EOF

tap3 'using -I "" to disable' <<'EOF'
xe -I "" echo {} x %
<<<
1
2
3
>>>
{} x % 1
{} x % 2
{} x % 3
EOF

tap3 'using {} with multiple arguments' <<'EOF'
xe -N2 echo a {} x {}
<<<
1
2
3
>>>
a 1 2 x {}
a 3 x {}
EOF

tap3 'using -0' <<'EOF'
printf "foo\0bar\0quux" | xe -0 echo
>>>
foo
bar
quux
EOF

tap3 'using -a' <<'EOF'
xe -a echo -- 1 2 3
>>>
1
2
3
EOF

tap3 'using -a with no arguments' <<'EOF'
xe -a echo
>>>
EOF

tap3 'using -a with no command' <<'EOF'
xe -N2 -a -- 1 2 3
>>>
1
2
3
EOF

tap3 'using -A%' <<'EOF'
xe -A% echo -- % 1 2 3
>>>
-- 1
-- 2
-- 3
EOF

tap3 'using -A% with no arguments' <<'EOF'
xe -A% echo
>>>2
xe: '-A %' used but no separator '%' found in command line.
EOF

tap3 'using -A% with no command' <<'EOF'
xe -N2 -A% % 1 2 3
>>>
1
2
3
EOF

tap3 'using -f' <<'EOF'
echo notme | xe -f Makefile echo
>>> /DESTDIR/
EOF

tap3 'using -s' <<'EOF'
xe -s 'echo x$1'
<<<
1
2
3
>>>
x1
x2
x3
EOF

tap3 'using -s with -N0' <<'EOF'
xe -N0 -s 'echo x$@'
<<<
1
2
3
>>>
x1 2 3
EOF

tap3 'using -s with -a' <<'EOF'
xe -s 'echo x$@' -a 1 2 3
>>>
x1
x2
x3
EOF

tap3 'using -s with -a' <<'EOF'
xe -a -s 'echo x$@' 1 2 3
>>>
x1
x2
x3
EOF

tap3 'using -s with -a' <<'EOF'
xe -a -s 'echo x$@' -- 1 2 3
>>>
x1
x2
x3
EOF

tap3 'with ITER' <<'EOF'
xe -a -s 'echo $ITER' -- a b c
>>>
1
2
3
EOF
