# Test ternary macro and its quirks.
# Expect:
# 12.
# 3.

println: null ? 1 : 2 + : true ? 10 : 20

# If you accidentally use C++ syntax, something bad happens.
# TODO: Colon and ? should work together
let .x ^a{a + 2}
println: true ? x 1 : 2
