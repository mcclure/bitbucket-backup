# Test Object super-- sort of a scope test

# Basic super test
# Expect: 10 12 15

let obj1 = inherit Object
	v = 1
	method f = function()
		10

let obj2 = inherit obj1
	v = 2
	method f = function()
		+
			current.v
			super.f()

let obj3 = inherit obj2
	v = 3
	method f = function()
		let inner = inherit obj1
			f = function()
				super.f()     # obj3's super-- this is not what a reasonable user would expect
		+
			current.v
			inner.f()

print
	obj1.f()
	obj2.f()
	obj3.f()
	ln

# Same test, but have the method use this
# Expect: 1 20 210

let obj4 = inherit Object
	v = 1
	method f = function()
		this.v

let obj5 = inherit obj4
	v = 10
	method f = function()
		+
			current.v
			super.f()

let obj6 = inherit obj5
	v = 100
	method f = function()
		let inner = inherit obj1
			f = function()
				super.f()     # obj5's super-- this is not what a reasonable user would expect
		+
			current.v
			inner.f()

print
	obj4.f()
	obj5.f()
	obj6.f()
	ln
