# Minimal compileable -- simple math and function call
# Expect:
# 7
# 5

# Tags: compiler

profile experimental

let x = 3
let y = 4

let z = x + y
let q = (z + x * y + 1) / (y + x - x)

println z
println q
