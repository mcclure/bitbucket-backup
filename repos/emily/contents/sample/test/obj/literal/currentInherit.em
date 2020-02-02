# Test the "current" binding with inheritance within an object literal.
# Expect:
# 3.

let .a [ current .let .b 3 ]
let .b [
    let .parent a
    println( current.b )
]
