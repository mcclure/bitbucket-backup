# Test iterators

# Expect:
# 1 2 3
# 3 2 1

let x = array
	1
	2
	3

let i = x.iter

while (i.more)
	let z = i.next
	print z
print ln

let i2 = x.reverseIter

while (i2.more)
	let z = i2.next
	print z
print ln
