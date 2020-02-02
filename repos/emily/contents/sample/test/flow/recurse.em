# Test complex ternary expressions and a recursive function. Expected output: numbers 9 through 0, descending.

# Expect:
# 9.
# 8.
# 7.
# 6.
# 5.
# 4.
# 3.
# 2.
# 1.

# Would be better if it short circuited!
let .or ^a( ^b( tern ^(a) ^(a) ^(b) ) )

let .countdown ^x{
    set .x (x .plus (0 .minus 1))
    tern ( or (x .gt 0) (x .eq 0) )  ^( println x; countdown x ) ^( null )
}

countdown 10