# Minimal if with weird looking cond

# Tags: compiler

# Expect:
# 995
# 1
# 3

profile experimental

let x = 0
let y = 0

# TODO where's the then statement
if (
	do
		let z = x < 4
		if (z)
			x = x + 1
		x > 0
)
	println 995 # "True"
	y = 3
else
	println 994 # "False"
	y = 4

println x
println y