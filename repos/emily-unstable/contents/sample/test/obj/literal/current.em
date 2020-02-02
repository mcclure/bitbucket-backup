# Test the "current" binding within an object literal.
# Expect:
# 3.

let .a [ current .let .b 3 ]

println ( a.b )
