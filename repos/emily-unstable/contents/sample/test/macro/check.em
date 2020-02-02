# Test the pseudo-boolean "check" operator
# Expect:
# <null>
# <object>
# 5.
# <object>
# 3.
# 8.
# OK
# 3.

x = [ y = [ z = 3 ] ]

println: y//
println (x // 4) (y // 5) (x.y // 6) (x.y.z // 7) (x.y.q // 8)

# Let's make sure the order of operations is what one would expect.
println: j // 3 ? null || x.c // "OK" : "NO"
println: x.y.j // p // x.y.z // 9