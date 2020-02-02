# Test assignment macro with constant keys.

# This should not be legal, but it is :(
0 = 4

# Store something in index 0 then print it back
# Expect: 3.

x = [0 = 3; 1 = 4]
println: x 0

# Expect:
# 3.
# 4.
# <null>
x.each println
println: x.has.count

# Save an atom in a variable then store to that key.
# Expect:
# 5.
# 6.
# 2.

countKey = .count
y = [0 = 5; 1 = 6; (countKey) = 2]
y.each println
println: y.count
