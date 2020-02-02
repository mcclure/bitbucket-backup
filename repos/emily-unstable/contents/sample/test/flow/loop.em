# Test loop construct. Expected output: numbers 10 through 1, descending.

# Expect:
# 10.
# 9.
# 8.
# 7.
# 6.
# 5.
# 4.
# 3.
# 2.
# 1.

let .a 10

loop ^(
    println a
    set .a (a .minus 1)
    a.gt 0
)