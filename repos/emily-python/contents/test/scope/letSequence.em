# Test scope assignment
# Expect:
# 1
# 2
# 3
# 4
# 5
# 4
# 4
# 2
# 2

let x = 1
println x

do
	x = 2
	println x

	do
		let x = 3
		println x

		do
			x = 4
			println x

			do
				let x = 5
				println x
			println x
		println x
	println x
println x

