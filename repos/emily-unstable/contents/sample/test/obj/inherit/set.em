# Test "this" binding with inherited setter
# Expect:
# 2.

let .a [
    let .b 1
]

let .c [
    let .parent a
]

c.set.b 2

println( a.b )
