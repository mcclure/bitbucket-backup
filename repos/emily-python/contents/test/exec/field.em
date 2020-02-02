# Test instances (ie new and fields)

# Basic fields and scoping
# Expect:
# 1 2 3 4 5 10 3
# 1 7 9 4 8 10 9

let a = inherit Object
	field a = 1
	field b = 2
	field c = 3
	x = 4
	y = 5
	z = 6
	method cProperty = this.c

let b = new a
	b = 7
	y = 8

b.c = 9
b.z = 10

print
	a.a, a.b, a.c
	a.x, a.y, a.z
	a.cProperty
	ln
	b.a, b.b, b.c
	b.x, b.y, b.z
	b.cProperty
	ln

# Numeric key aliases
# Expect: 3 100
print
	a 2

a 2 = 100

print
	a 2
	ln

# Positional key assignment (Notice: 100 as a leftover effect of key alias test)
# Expect: 201 203 100

let c = new a
	201
	202

c 1 = 203

print
	c 0, c 1, c 2
	ln

# Quirks: The self-hosted interpreter choked on this once because it wasn't properly copying things.
# Expect: 1
let A = inherit Object

let B = inherit A
	field c = null

let D = inherit B
	field e = 0
	
let F = inherit B

let G = new F(1)

println
	1.0
