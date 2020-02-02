# Test quirks of variable shadowing in methods.
# Sincerely not sure what I expect this to do.
# Expect:

let .var 3

let .x ^var (
    println var
    let .var 4
    println var
)

let .y ^var {
    println var
    let .var 5
    println var
}

x 6
println var
y 7
println var