# Test loop construct. Expected output: numbers 10 through 1, descending.

set .a 10

loop ^(
	print a
	print "\n"
	set .a (a .minus 1)
	a.gt 0
)