# Minimal function test, disambiguation on addOne call
# Expect: 16

# Tags: compiler

profile experimental

let x = 2
let addOne = function(a, x)
	a + x + 1

println
	10 + addOne
		x
		3
