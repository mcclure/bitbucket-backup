# Test string ops

let x = "one two three"

let i = x.iter

# Expect:
# oh n o n false x 13 3q 4 atom true 0 2
# e e r

print
	"oh".toString
	x 1
	i.next
	i.next
	"".iter.more
	"xyz".iter.next
	x.length
	+ (3..toString) "q"
	+ ("3".toNumber.toNumber.toString.toNumber) 1
	.atom.toString
	true.toString
	false.toNumber
	+ (true.toNumber) 1
	ln

let i2 = x.reverseIter

print
	i2.next, i2.next, i2.next
	ln
