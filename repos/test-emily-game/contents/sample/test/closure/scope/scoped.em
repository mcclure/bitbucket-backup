# Test variable binding for a scoped closure.
# Expect:
# 4.
# 3.

let .b 3

^{
    let .b 4
    println b
} null

println b