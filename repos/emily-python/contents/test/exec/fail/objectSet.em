# Test object ops [assign to nonexistent key]
# Expect failure

let x = inherit Object
	a = 3

x.b = 4
