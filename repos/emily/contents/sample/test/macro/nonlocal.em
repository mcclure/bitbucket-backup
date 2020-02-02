# Test assignment macro w/"nonlocal".
# Expect:
# 9.
# 7.

a = 3

# This is pretty awkward: the "nonlocal" tag is the inverse of the "let" tag.
# It means ultimately the assignment is based on set rather than let.
{
    nonlocal a = 7 # So this falls through,
    a = 8          # This creates a new a binding in this scope,
    nonlocal a = 9 # This catches on the a binding in this scope.
    println a
}
println a
