# "let"s in this language, currently, are not legit lets and should be named "var". Prove this is true:
# Expect: 5

let x = 3

let xf = function () (x)

let x = 5

println
	xf()
