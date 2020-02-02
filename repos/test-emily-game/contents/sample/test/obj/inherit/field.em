# Test "this" binding with inheritance
# Expect:
# 1.

let .a [
    let .b 1
]

let .c [
    let .parent a
]

println( c.b )
