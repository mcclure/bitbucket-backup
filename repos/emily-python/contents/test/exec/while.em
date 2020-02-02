# Test loop

# Expect: 3628800

let x = 10
let y = 1

while (> x 0)
	y =
		* y x
	x =
		- x 1

println y

