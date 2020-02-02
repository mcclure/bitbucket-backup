# Test weird implications of object literal scoping.
# Expect:
# 4.
# 3.
# 5.
# 6.
# 7.

let .b 2
let .a [
    let .b 3
    set .b 4
    let .c ^(
        set .b 5
        current .set .b 6
        let .d 7
    )
]

println ( b )
println ( a.b )
a.c null
println ( b )
println ( a.b )
println ( a.d )
