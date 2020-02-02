# Test private within an object literal.
# Should change in lockstep with package/private and package/inner/private
# Expect:
# (Inside)
# 2.
# 7.
# 4.
# 4.
# <true> <true> <true>
# <null> <null>
# <true> <true> <true> <null>
# <null> <true> <true> <null>
# <null> <true> <null> <true>
# (Outside)
# 7.
# 3.
# <null> <null> <null>
# <null> <null> <true>

global = 1
shadowed = 2

obj = [
    println: "(Inside)"

    # Test scope fallthrough-- variables just check enclosing scope
    println: shadowed

    # Test nonlocal set works as expected-- should fall through to outer scope
    nonlocal shadowed = 7

    # Test object member assignment
    shadowed = 3

    # Test scope fallthrough-- we don't search the object, but do search the parent (which we've reassigned)
    println: shadowed

    # Test assigning private variable
    private.shadowed = 4

    # Test readback in this scope checks private before enclosing
    println: shadowed

    # Test private may be queried directly
    println: private.shadowed

    # Test assigning object variable with unique name, for has test
    visible = 5

    # Test assigning private variable with unique name, for has test
    private.hidden = 6

    # Test has doesn't get confused around the "invisible" box scope
    print (has .let) sp (has .private) sp (has .current) ln

    # Test a couple weird places private could (but shouldn't) leak
    print (private.has .private) sp (current.has .private) ln

    # Test we see both private and global variables in basic scope, but don't see global in private scope
    print (has .global)         sp (has .shadowed)         sp (has .hidden)         sp (has .visible)         ln \
          (private.has .global) sp (private.has .shadowed) sp (private.has .hidden) sp (private.has .visible) ln \
          (this.has .global)    sp (this.has .shadowed)    sp (this.has .hidden)    sp (this.has .visible)    ln
]

println: "(Outside)"

# Read back global scope
println: shadowed

# Read back object member
println: obj.shadowed

# Ensure privates from literal didn't leak out into enclosing scope
print (has .hidden)     sp (has .private)    sp (has .visible)      ln

# Ensure privates from literal didn't leak out into object
print (obj.has .hidden) sp (obj.has .private) sp (obj.has .visible) ln