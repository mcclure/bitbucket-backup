# Test late field definition on an object.
# Expect:
# 3.

let .a []
a.let.b 3
println(a.b)