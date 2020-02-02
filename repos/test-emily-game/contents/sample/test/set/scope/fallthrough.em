# Test a scoped group.
# Expect:
# 4.
# 4.

let .b 3

{
    set .b 4
    println b
}

println b