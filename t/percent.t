#!/bin/sh
export "PATH=.:$PATH"

printf '1..13\n'
printf '# percent rules\n'

tap3 'literal matches' <<'EOF'
xe -ap bcd echo found -- abc bcd defg
>>>
found
EOF

tap3 'multiple patterns' <<'EOF'
xe -ap one echo 1 + two echo 2 + three echo 3 -- zero one two three four five
>>>
1
2
3
EOF

tap3 '{} expansion' <<'EOF'
xe -ap bcd echo {} -- abc bcd defg
>>>
bcd
EOF

tap3 '% expansion' <<'EOF'
xe -ap bcd echo % -- abc bcd defg
>>>
bcd
EOF

tap3 'dirnames' <<'EOF'
xe -ap bcd echo % -- abc bcd /tmp/bcd /tmp/abc
>>>
bcd
/tmp/bcd
EOF

tap3 '? glob' <<'EOF'
xe -ap "b?d" echo % -- abc bcd b3d defg
>>>
bcd
b3d
EOF

tap3 '* glob' <<'EOF'
xe -ap "b*d" echo % -- bd bed bad bugged bx zbd b/d
>>>
bd
bed
bad
bugged
EOF

tap3 'multiple * glob' <<'EOF'
xe -ap "b*g*d" echo % -- bd bed bugged bx zbd bagdad badger ba/gd/ad
>>>
bugged
bagdad
EOF

tap3 'multiple ** glob' <<'EOF'
xe -ap "b**g**d" echo % -- bd bed bugged bx zbd bagdad badger ba/gd/ad
>>>
bugged
bagdad
ba/gd/ad
EOF

tap3 '/ slash' <<'EOF'
xe -ap a/b echo 1 + c///d echo 2 + "*" echo 3 -- a/b a//b a/ b c/d /c////d
>>>
1
1
3
3
2
3
EOF

tap3 '[] ranges' <<'EOF'
xe -ap "[abc]" echo "1%" + "[d-g]" echo "2%" + "[^xyz-]" echo "3%" + "[!-vw]" echo "4%" + % echo "5%" -- a c d e g h w x -
>>>
1a
1c
2d
2e
2g
3h
3w
4x
5-
EOF

tap3 '{} alternation' <<'EOF'
xe -ap "{a,bc,def*}" echo % -- x a abc bc bcd def defx xdef
>>>
a
bc
def
defx
EOF

tap3 '% match' <<'EOF'
xe -ap %.c echo obj/%.o -- foo.c bar.cc meh/quux.c
>>>
obj/foo.o
meh/obj/quux.o
EOF
