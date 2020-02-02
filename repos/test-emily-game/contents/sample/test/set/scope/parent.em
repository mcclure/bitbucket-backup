# Test parent field on scopes
# Expect:
# 5.
# 6.

let .x 3

{
    let .x 4
    set .x 5
    parent.set.x 6
    println x
}

println x
