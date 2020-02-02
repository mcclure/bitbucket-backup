# Test user-defined operators
# Expect:
# 24
# 24
# -9
# 5

let add = \+
let subtract = \-
let times = \*

# Inside this block, use ALGOL-like operator precedence
let x = do 
	macro
		splitMacro(600, "add")
		splitMacro(610, "times")

	3 times 4 add 6 times 2

# Back outside the block, everything's LISP again
let y =
	add (times 3 4) (times 2 6)

# Test "leftward" (flow control style) precedence
let l = do 
	macro
		splitMacro(600, "add")
		splitMacro(600, "subtract")

	3 subtract 5 add 7

# Test "rightward" (arithmetic style) precedence
let r = do 
	macro
		splitMacro(601, "add")
		splitMacro(601, "subtract")

	3 subtract 5 add 7

println
	x
	y
	l
	r
