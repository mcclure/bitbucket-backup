# Test quirk: What does a = b evaluate to?

# Note, I don't actually care what this is and am not sure I'm ready to freeze it in the lang spec,
# but the self-hosting interpreter depends on it being *some* non-ridiculous value

# Expect: null

let a = 1
println
	a = 3
