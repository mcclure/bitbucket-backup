# Minimal compileable with loop and if
# Expect file: test/compiler/loopOutput.txt

# Tags: compiler

profile experimental

let x = 1

while (x <= 100)
	if (x % 3 == 0 && x % 5 == 0)
		println "FizzBuzz"
	elif (x % 3 == 0)
		println "Fizz"
	elif (x % 5 == 0)
		println "Buzz"
	else
		println x

	x = x + 1
