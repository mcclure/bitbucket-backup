# Test import * behaves as expected

# Expect: 3 30 3 20 3 20

let z = 10

let a = inherit Object
	b = 3
	method c = z

z = 20

let d = inherit Object
	from a import *

z = 30

from d import *

print
	a.b
	a.c
	d.b
	d.c
	b
	c
	ln

# Test packages in addition to objects
# Expect: 103 105

from project.fail.localPackage.includeMe import *

print
	e
	f
	ln
