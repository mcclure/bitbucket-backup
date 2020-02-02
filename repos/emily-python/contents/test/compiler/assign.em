# Minimal compileable -- assume blocks but no math or fn calls

# Tags: compiler

profile experimental

let x = 2
let y = 3
let z = x

# Returns final value (x) and assigns it to a
let a = do
	x = y
	z = x
	do
		let q = x
		q
