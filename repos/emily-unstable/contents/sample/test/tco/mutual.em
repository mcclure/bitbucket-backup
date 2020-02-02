# Test that tail call recursion works and does not infinitely grow the stack or anything.
# (Version 2: Mutual recursion)
# NOT PART OF REGRESSION TESTS -- DOES NOT TERMINATE

let .prints 0

let .recurse ^ x {
    # Waste some space
    let .a (x .minus 1)
    let .b (x .minus 2)
    let .c (x .minus 3)
    let .d (x .minus 4)
    let .e (x .minus 5)

    tern (x .gt 10000) ^(
        print "So far printed " prints " times"; println
        set .x 0
        set .prints (prints .plus 1)
    ) ^(null)

    tern (0 .gt x) ^( recurse (0 .minus (x .plus 1)) ) ^( recurse2 x )
}

let .recurse2 ^ x {
    recurse x
}

recurse 0
