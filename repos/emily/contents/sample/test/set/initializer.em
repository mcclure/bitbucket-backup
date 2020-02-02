# Test the group initializer feature.

x = [ q = [ a = 1; b = 2 ] ]

# Expect: 1.
println: x.q.a

# Expect: 3.
println { x.q | a = 3; a }

# Expect: 1.
println: x.q.a

# Expect: 5.
println ( x.q | a = 5; a )

# Expect: 5.
println: x.q.a

# Expect: 7.
println: [ x.q | a = 7 ].a

# Expect: 7.
println: x.q.a

# One last thing: Make sure that this/super work correctly with object initializers.
# Expect:
# 7.
# 2.

[x.q | test ^ = println: this.a]

y = [ parent=x.q ]

[y | test ^ = (
    do: super.test
    println: this.b
)]

do: y.test