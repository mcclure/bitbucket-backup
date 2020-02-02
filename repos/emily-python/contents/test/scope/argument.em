# Test function argument scope treatment

# Expect:
# 3
# 6

let y = 6
let q = 7

let x = function (y) (
	let q = y
	y = 4
	q
)

println (x 3)
println (y)
