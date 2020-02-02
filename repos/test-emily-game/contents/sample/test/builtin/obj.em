# Test object base class methods.
# Expect:
# Count: 4.
# 10.
# 6.
# 4.
# 2.

let .array [
]

array.append 5
array.append 3
array.append 2
array.append 1

array = array.map ^i (i*2)

print "Count: "  (array.count) ln
array.each ^i (println i)