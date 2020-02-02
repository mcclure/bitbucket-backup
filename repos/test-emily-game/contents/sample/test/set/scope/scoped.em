# Test a scoped group. Expected output: 4.0 [newline] 3.0
# Expect:
# 4.
# 3.

let .b 3

{
    let .b 4
    println b
}

println b