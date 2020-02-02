# Minimal if test

# Tags: compiler

# Expect:
# 999
# 997
# 1

profile experimental

let x = 0
let y = x < 5

if (y)
	println 999 # "True"
	x = 1
else
	println 998 # "False"
	x = 2

if (y)
	println 997 # "True2"

let z = x > 1

if (z)
	println 996 # "True3"

println x