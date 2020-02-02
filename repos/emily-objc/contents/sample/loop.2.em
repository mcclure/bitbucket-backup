# Test loop construct plus argument blocks. Expected output: numbers 10 through 18, up by twos.

set .countup ^arg{
	set .count (arg.from)
	loop ^(
		print count
		print "\n"
		set .count ( count .plus (arg.step) )
		count.lt (arg.to)
	)
}

countup[ set .from 10; set .to 20; set .step 2 ]