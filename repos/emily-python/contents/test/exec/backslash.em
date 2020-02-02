# Just totally nail down that the backslash parser acts as expected
# Expect:
# 3
# 5
# 5

let \method = 3

println \method

let \let = null
\let = inherit Object
	\method = 5

println
	\let.method
	\let \.method

# Test import specially
# Expect:
# 5
# 3

let z = inherit Object
	\* = 5
	\method = 6

from z import \*

println
	\*
	\method
