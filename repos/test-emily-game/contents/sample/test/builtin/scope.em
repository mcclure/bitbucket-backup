# Test all random library (builtinScope) functions that don't have their own tests

# Test sp, ln

# Expect: a b
print "a" sp "b" ln

# Test printsp

# Expect:
# a b   c
# d
printsp "a" "b" " " "c" ln "d" ln

# Test do

# Expect: 8.
do ^(println 8)

# Test nullfn, true

tern true nullfn ^{ println 9 }

# Test if, null

# Expect: 7.
if (3)    ^{println 7}
if (null) ^{println 6}

# Test loops
# loop tested in loop.em

# Expect:
# 3.
# 2.
# 1.
let .i 3
while ^(i.gt 0) ^{println i; set .i (i .minus 1)}

# Expect:
# 0.
# 1.
# 2.
upto 3 println

# Test and, or, xor

# Expect: <null>
println ( xor ^(or ^( and ^(true) ^(null) ) ^(true) ) ^(true) )

# Test check

# Expect: 3. 4. 5. 6.
let .i 3
let .q [x=5]
print (check scope .i ^(4)) sp (check scope .j ^(4)) sp \
      (check q     .x ^(6)) sp (check q     .y ^(6)) ln

# Test math

# Expect: 2. 2. 2. 3.
print (floor 2)   sp (floor 2.718281828) sp \
      (ceiling 2) sp (ceiling 2.718281828) ln

# Test types

# Expect: <true> <null> <true> <null> <true> <null> <true> <null>
print (atom .atom)       sp (atom "atom")          sp \
      (string "3.14159") sp (string 3.14159)       sp \
      (number 3.14159)   sp (number "3.14159")     sp \
      (int 3)            sp (int 3.14159)          ln

# Test object

# Expect: 3. 4. 3. 5.
q = [x = 3; y = 4]
r = inherit q; r.y = 5
print (q.x) sp (q.y) sp (r.x) sp (r.y) ln

# Test scope manipulation

# Expect: 3.

a = 3
println: scope.a

# Expect: 4.

scope.a = 4
println: a