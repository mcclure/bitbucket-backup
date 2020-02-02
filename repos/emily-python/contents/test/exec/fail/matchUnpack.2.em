# Test match with too many fields in untyped unpack
# Expect failure

let None = inherit Object
let Some = inherit Object
	field value = null

let p = new Some(3)

with p match # Unpack vector
	None = println "?"
	array(a, b, c) = print a b c ln
