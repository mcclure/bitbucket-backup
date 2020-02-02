# Arg: --ast
# Expect: (Zero, Zero, One two (three four (five), six), One two (three four (five, six)), One (two three) (four (five (six)) seven (eight nine), ten), One (two (three), four (five)))

Zero

Zero
One two
	three four
		five
	six

One two
	three four
		five
		six

One (two three)
	four (
five
	six
	) seven
		eight nine
	ten

One (
	two
		three
	four
		five
)