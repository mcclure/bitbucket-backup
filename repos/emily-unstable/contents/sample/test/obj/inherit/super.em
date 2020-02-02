# Test "this" binding with method call redo
# Expect:
# 3.
# 2.
# 3.
# 4.

let .obj1 [
    let .var1 1
    let .var2 2
    let .meth ^{
        # When called w/super this should invoke obj2.set, which sets obj1.var1
        this.set.var1 3
        # ...and this should invoke obj2.set, then sets obj2.var2.
        this.set.var2 4
    }
]

let .obj2 [
    let .parent obj1
    let .var2 5  # Shadow
    let .meth ^{ # Overrides obj1.meth
        this.set.var1 6 # This sets obj1.var1
        this.set.var2 7 # This sets obj2.var2
        super.meth null
    }
]

# Expected result: Resolves to obj2.meth, which invokes obj1.meth
obj2.meth null

println( obj1.var1 )
println( obj1.var2 )
println( obj2.var1 )
println( obj2.var2 )
