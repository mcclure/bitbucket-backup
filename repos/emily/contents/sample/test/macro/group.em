# Test some grouper macros.
# Expect:
# 7.
# 7.
# c: 3.
# b: 4.
# a: 5.
# 6.

println: 3 + 4; println: 3 .plus: 4

let .a ^x { print "a: " x ln; x+1 }
let .b ^f x {
    let .y `f x
    print "b: " y ln
    y+1
}
let .c ^x { print "c: " x ln; x+1 }

# Apply b to c, apply result to 3, apply a to result, print result
# Which has the effect: b invokes c on 3, prints result, feeds result to a
print ` a ` ` b c 3 ln