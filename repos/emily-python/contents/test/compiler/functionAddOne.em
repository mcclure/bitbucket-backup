# Minimal function test
# Expect: 6

# Tags: compiler

profile experimental

let x = 2
let addOne = function(a, x)
	a + x + 1

println
	addOne
		x
		3
