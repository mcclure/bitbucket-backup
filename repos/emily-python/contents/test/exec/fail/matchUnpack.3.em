# Test match with mis-specified object
# Expect failure

let None = inherit Object
let Some = inherit Object
	field value = null

let p = None

with p match # Unpack vector
	None(a) = println "?"
	Some(a) = println a
