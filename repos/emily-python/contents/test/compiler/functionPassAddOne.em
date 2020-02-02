# Minimal function test w/higher order function
# Expect: 116

# Tags: compiler

profile experimental

let x = 2
let addOne = function(a, x)
	a + x + 1
let invokeAndAddTen = function(f, a, x)
	f(a, x) + 10

println
	100 + invokeAndAddTen
		addOne
		x
		3
