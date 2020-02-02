# Test a scoped group. Expected output: 4.0 [newline] 3.0

set .b 3

{
	set .b 4
	print b
}

print "\n"
print b