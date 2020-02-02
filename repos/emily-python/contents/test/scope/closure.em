# Test closure memory

# Expect:
# 1
# 101
# 3
# 103
# 6
# 106

let generator = function (x)
	let counter = x
	let generated = function (x)
		counter =
			+ counter x
		counter
	generated

let one = generator 0
let two = generator 100

println
	one 1
	two 1
	one 2
	two 2
	one 3
	two 3
