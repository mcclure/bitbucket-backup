# Test "this" binding
# Expect:
# 3.

let .a [
    let .b 1
    let .c ^d{ this.set.b (this.b .plus d) }
]

a.c 2
println( a.b )
