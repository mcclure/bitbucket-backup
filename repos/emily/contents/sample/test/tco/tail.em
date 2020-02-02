# Test that tail call recursion works and does not infinitely grow the stack or anything.
# NOT PART OF REGRESSION TESTS -- DOES NOT TERMINATE

let .prints 0

let .v ^{v}
let .y 0

let .recurse ^ x {
    # Waste some space
    let .a (x .minus 1)
    let .b (x .minus 2)
    let .c (x .minus 3)
    let .d (x .minus 4)
    let .e (x .minus 5)

    tern (x .gt 10000) ^(
        set .y (y .plus x)
        set .x 0
        println "So far printed " y "times"
    ) v

    recurse (x .plus 1)
}

recurse 0