# Test if statement

# Expect:
# two
# x false
# y true

let x = 0
let y = 2

if x
	println "zero"
if y
	println "two"

println
	if x ("x true") else ("x false")

println
	if y ("y true") else ("y false")

# Expect:
# w true
# all false
# w true
# all false

let z = 0
let w = 1
let v = 0

println
	if x ("x true") elif z ("z true") elif w ("w true") else ("all false")

println
	if x ("x true") elif z ("z true") elif v ("v true") else ("all false")

if x
	println "x true"
elif z
	println "z true"
elif w
	println "w true"
else
	println "all false"

if x
	println "x true"
elif z
	println "z true"
elif v
	println "v true"
else
	println "all false"

# Expect:
# null

println
	if 0 (4)
