# Test array ops [assign to nonexistent key]
# Expect failure

let x = array
	1
	2

x 10 = 10
