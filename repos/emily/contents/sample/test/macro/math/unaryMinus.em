# Test unary minus, which is weird.

# Expect:
# -4.
# -1.
# 7.
# -12.
# -0.75
# -1.
# minus
# 4.

println: -4
println: 3 + -4
println: 3 - -4
println: 3 * -4
println: 3 / -4
println: 3 % -4

# Test we do something obnoxious here
println - 4