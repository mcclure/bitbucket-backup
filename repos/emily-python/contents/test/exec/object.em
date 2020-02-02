# Test object ops

# Test object creation
# Expect:
# 3
# 4
# 5

let obj1 = inherit Object
	a = 1
	b = 2
	c = 3

let obj2 = inherit obj1
	c = 4
	d = 5

println
	obj1.c
	obj2.c
	obj2.d

# Test object creation with dynamic values
# Expect:
# 3
# 10
# 3
# 20

let counter = 0
while (< counter 2)
	counter =
		+ counter 1
	let obj3 = inherit obj2
		c =
			* counter 10
	println
		obj1.c
		obj3.c

# Test object key assignment
# Expect:
# 10
# 11

obj2.c = 10
let obj1.e = 11

println
	obj2.c
	obj2.e

# Test object key assignment with dynamic keys
# Expect:
# 20
# 21

let key = if 1 (.c) else (.d)
obj2 key = 20.0
obj2 (if null (.c) else (.d)) = 21.0

println
	obj2.c
	obj2.d

# Test nested assignment. While we're at it test creation of an empty object
# Expect:
# 30

obj2.c = inherit Object
	x = inherit Object
		y = null

obj2.c.x.y = inherit Object()
let obj2.c.x.y.z = inherit Object
	w = 0
obj2.c.x.y.z.w = 30.0

println
	obj2.c.x.y.z.w

# Test nested, dynamic key assignment. I acknowledge this section is gross
# Expect:
# 40

key = .y
obj2.c.x key .z.w = 40.0
key = .x
println
	obj2.c key .y.z.w

# Test current
# Expect:
# 4 6

let obj3 = inherit Object
	x = 3
	y = + (current.x) 1
	method z = + (current.y) 2

let obj4 = inherit obj3
	y = 10

print(obj3.y, obj4.z, ln)
