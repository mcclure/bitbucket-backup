# Test import behaves as expected

let a = inherit Object
	b = 1
	c = 2
	d = inherit Object
		e = 3
		f = 4
		g = 5

import a.b
from a import c
from a import d.e

from a.d import
	f, g

# Expect: 1 2 3 4 5

print
	b
	c
	e
	f
	g
	ln
