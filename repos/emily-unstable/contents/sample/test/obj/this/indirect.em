# Verify method invocation has same behavior with `this` regardless of direct/indirect invoke
# Expect:
# 1.
# 1.
# 1.

let .obj1 [
    let .var1 1
    let .meth ^{ println(this.var1) }
]

# Normal invoke
obj1.meth null

# Indirect invoke through builtin
do (obj1.meth)

# Indirect invoke through method
let .fDo ^f { f null }

fDo (obj1.meth)