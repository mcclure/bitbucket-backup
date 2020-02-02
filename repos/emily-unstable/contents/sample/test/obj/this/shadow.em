# Test "this" and "current" bindings shadow environment, but not arguments
# Expect:
# <object>
# <object>
# 5.
# 6.

let .current "X"
let .this    "Y"

let .a [
    let .b ^ curren  thi  { println current this }
    let .c ^ current this { println current this }
]

a.b 3 4
a.c 5 6