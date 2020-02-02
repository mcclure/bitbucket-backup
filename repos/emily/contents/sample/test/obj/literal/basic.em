# Test an object literal.
# Expect:
# 3.

let .a [ let .b 3 ]

println ( a.b )
