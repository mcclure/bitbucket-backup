# Test variable shadowing for an unscoped closure.
# Expect:
# 4.
# 3.

let .b 3

^b(
    println b
) 4

println b