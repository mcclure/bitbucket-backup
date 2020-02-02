# Demonstrate modulus and its behavior wrt signs

# Expect:
# 1.
# -3.
# 3.
# -1.

println ( ( 5         ) .mod ( 4         ) )
println ( ( 5         ) .mod ( 4 .negate ) )
println ( ( 5 .negate ) .mod ( 4         ) )
println ( ( 5 .negate ) .mod ( 4 .negate ) )