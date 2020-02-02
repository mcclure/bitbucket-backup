# Test this-hackery functions: thisTransplant, thisInit, thisFreeze, thisUpdate
# Expect:
# Transplant
# 2. 2.
# 3. 3.
# Init
# 2. 2.
# 3. 3.
# 3. 3.
# 3. 3.
# Update
# 2. 3.
# 3. 4.
# 2. 3.
# 3. 2.
# 4. 2.
# Freeze
# 3. 3.
# 2. 2.

sp = " "

obj2 = [
    var1 = 2

    # obj2 and obj3 each have copies of this same "meth" function.
    # It identifies, effectively, the values of current and this.
    meth ^ = print `current.var1 sp `this.var1 ln

    # Test thisFreeze: Make a method which captures local this,
    # and one which is "this-blank" (will adopt parent this if transplanted)
    makeTestFreeze1 ^ =            ^( do: this.meth )
    makeTestFreeze2 ^ = thisFreeze ^( do: this.meth )
]

obj3 = [
    var1 = 3

    # Here's that "meth" function again
    meth ^ = print `current.var1 sp `this.var1 ln

    # Test 1 copies in obj2's meth, the act of copying freezes it, should print 2 2
    # Test 2 copies obj2's meth but transplant makes it obj3's now, should print 3 3
    testTransplant1 =                 obj2.meth
    testTransplant2 = thisTransplant: obj2.meth

    # Test 1 gets a new blank method coincidentally created in an obj2 method scope;
    #     this method becomes obj3's. Output: 3 3
    # Test 2 gets a new method but it's frozen, it never becomes a method, it captures
    #     a scope from an obj2 method & the "this" in that scope is used. Output: 2 2
    testFreeze1 = do: obj2.makeTestFreeze1
    testFreeze2 = do: obj2.makeTestFreeze2

    # Test 1 will pull in obj2's meth, rewrite it as if obj3 inherited from obj2
    #     and the method was called as super; then the act of assigning freezes it
    #     Output: 2 3 because with super current and this become different
    testUpdate1 = thisUpdate this: obj2.meth
]

obj4 = [
    var1 = 4

    parent = obj3
]

# Same meth from obj2, obj3
nakedMeth ^ = print `current.var1 sp `this.var1 ln

# Test 1: Pull meth from obj2, it freezes, stays obj2's. Output: 2. 2.
# Test 2: Assign an after-the-fact method to become obj3's. Output: 3. 3.
# Test 3: Assign a method created inside obj2 to become obj3's. A little extraneous. Output: 3. 3.
obj3.testInit1 = obj2.meth
obj3.testInit2 = thisInit obj3: nakedMeth
obj3.testInit3 = thisInit obj3: do: obj2.makeTestFreeze1

# Test transplant
println "Transplant"
do: obj3.testTransplant1
do: obj3.testTransplant2

# Test init
# Test 4: Show that even though Test 2 is 100% 3's, super is now broken. Output: 3. 3.
println "Init"
do: obj3.testInit1
do: obj3.testInit2
do: obj3.testInit3
do: obj4.testInit2

# Test update
# Test 2: Plain inherits meth from obj3. Output: 3 4
# Test 3: Inherits testUpdate from obj3, but it's frozen, so still Output: 2 3
# Test 4: Manually impose obj3 as this on obj2 method. Output: 3 2
# Test 5: Just to show we can, turn obj3 meth into one with Output: 4 2
println "Update"
do: obj3.testUpdate1
do: obj4.meth
do: obj4.testUpdate1
do: thisUpdate obj2: obj3.meth
do: thisUpdate obj2: thisInit obj4: thisTransplant: obj3.meth

# Test freeze:
println "Freeze"
do: obj3.testFreeze1
do: obj3.testFreeze2