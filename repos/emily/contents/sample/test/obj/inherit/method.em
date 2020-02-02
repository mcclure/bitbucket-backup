# Test "this" binding with method call
# Expect:
# 3.

let .a [
    let .b 1
    let .c ^{ this.set.b 2 }
    let .d ^{ this.c null }
]

let .e [
    let .parent a
    let .c ^{ this.set.b 3 } # Overrides a.c
]

# Expected result: Resolves to a.d, which invokes .c,
# which resolves to e.c, which sets a.b to 3:
e.d null

println( a.b )
