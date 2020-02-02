# Minimal loop, no valued IfExec required
# Expect file: test/compiler/loopOutput.txt

# Tags: compiler

profile experimental

let x = 1

while (x <= 100)
	let three = x % 3 == 0
	let five = x % 5 == 0
	let both = false

	if (three)
		if (five)
			both = true

	if (both)
		println "FizzBuzz"
	elif (three)
		println "Fizz"
	elif (five)
		println "Buzz"
	else
		println x

	x = x + 1
