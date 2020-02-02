# Test complex ternary expressions and a recursive function. Expected output: numbers 9 through 0, descending.

# Would be better if it short circuited!
set .or ^a( ^b( tern ^(a) ^(a) ^(b) ) )

set .countdown ^x{
	set .x (x .plus (0 .minus 1))
	tern ^( or (x .gt 0) (x .eq 0) )  ^( print x; print "\n"; countdown x )  ^( null )
}

countdown 10