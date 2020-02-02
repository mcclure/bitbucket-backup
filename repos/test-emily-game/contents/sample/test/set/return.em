# What do set/let statements return?
# Expect:
# <null>
# 3.
# <null>
# 4.

println(let .x 3)
println(x)
println(set .x 4)
println(x)