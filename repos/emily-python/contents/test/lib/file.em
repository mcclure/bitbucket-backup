# File I/O

# Expect: true

let path = "/tmp/file.em.test.txt"

# This will be overwritten

let x = file.out path
x.println "OK"
x.close

# Write all these things to a file, then read them back and confirm they're the same

let print1 = "one two ⚧\n"
let print2 = "three four"
let print3 = "five"
let print4 = "six"
let printResult =
	+
		print1
		+
			print2
			+
				"\n"
				+
					print3
					+
						" "
						print4

let x = file.out path
x.write print1
x.println print2
x.close

let x = file.append path
x.print print3 print4
x.close

let sum = ""

let x = file.in path
while (x.more)
	sum =
		+
			sum
			x.next

println
	== sum printResult

x.close