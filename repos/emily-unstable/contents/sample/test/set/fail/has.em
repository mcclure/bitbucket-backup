# Test intersection of "has" with an incompletely applied let
# Expect:
# <null>
# <null>

println (has .a)
let .a
println (has .a)