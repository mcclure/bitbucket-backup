# Test short circuiting boolean operators

# Expect:
# GET FALSE
# false
# GET FALSE
# false
# GET TRUE
# GET FALSE
# false
# GET TRUE
# GET TRUE
# true
# GET FALSE
# GET FALSE
# false
# GET FALSE
# GET TRUE
# true
# GET TRUE
# true
# GET TRUE
# true

macro shortCircuitBoolean

let getFalse = function ()
	println "GET FALSE"
	null

let getTrue = function ()
	println "GET TRUE"
	1

# Awkward subtleness: Doubles as a test of "when" println prints
println
	getFalse() && getFalse()
	getFalse() && getTrue()
	getTrue()  && getFalse()
	getTrue()  && getTrue()
	getFalse() || getFalse()
	getFalse() || getTrue()
	getTrue()  || getFalse()
	getTrue()  || getTrue()