# Test that a "real" program can run. Prints the finonachi numbers.

# Expect:
# 1.
# 1.
# 2.
# 3.
# 5.
# 8.
# 13.
# 21.
# 34.
# 55.
# 89.

let .a 0
let .b 1

loop ^{
    println b

    let .c (a .plus b)
    set .a b
    set .b c

    b .lt 100
}
