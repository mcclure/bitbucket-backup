# Test array ops [assign to non-numeric key]
# Expect failure

let x = array
	1
	2

x.key = 10
