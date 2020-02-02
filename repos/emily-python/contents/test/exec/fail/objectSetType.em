# Test object ops [assign to string key]
# Expect failure

let x = inherit Object
	a = 3

let x "b" = 4
