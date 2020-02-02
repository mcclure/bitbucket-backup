# Test "this" inside a raw box definition
# Expect:
# 3.
# 3.
# 3.

let .a [
    let .b ^{println 3}
    current.b null
    this.b null
]

a.b null