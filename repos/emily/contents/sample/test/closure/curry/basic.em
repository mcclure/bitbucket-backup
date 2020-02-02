# Demonstrate a curried closure.
# Expect:
# 4.
# 5.

# Note: Significant argument order
let .a ^x y ( x .minus y )
let .b  (a 9)

println (a 7 3)
println (b 4)
