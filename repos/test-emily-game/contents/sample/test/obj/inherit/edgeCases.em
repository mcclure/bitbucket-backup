# Compare different ways of inheriting a method.
# FIXME: Is this *correct*? 7 surprises me.
# Expect:
# 2. 2.
# 2. 3.
# 2. 2.
# 2. 2.
# 2. 6.
# 2. 7.

sp = " "

# Normal object
obj2 = [
    var1 = 2

    # This identifies, effectively, the values of current and this.
    meth ^ = print `current.var1 sp `this.var1 ln
]

# Object inherits from obj2 normally -- "this" updates
obj3 = [
    var1 = 3

    parent = obj2
]

# Object inherits from obj2 through weird method
obj4 = [
    var1 = 4

    parent ^x = obj2 x
]

# Object inherits from obj3, which inherits through obj2 through weird method
obj5 = [
    var1 = 5

    parent = obj4
]

# Object inherits from obj2 through weird method + thisUpdate
obj6 = [
    var1 = 6

    parent ^x = thisUpdate this: obj2 x
]

# Object inherits from obj6, which inherits from obj2 through weird method + thisUpdate
obj7 = [
    var1 = 7

    parent = obj6
]

do: obj2.meth
do: obj3.meth
do: obj4.meth
do: obj5.meth
do: obj6.meth
do: obj7.meth