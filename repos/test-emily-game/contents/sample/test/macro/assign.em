# Test assignment macro.
# Expect:
# 3.
# 4.
# 5.
# 6.
# 7.
# 8.
# 9.

a = 3
b = []
b.x = 4
b a = 5
b (b.x) = 6
b.y = [a=7]
b.y.b = 8
b.y.c ^ x = ~x
b.y.d ^ x y = x - y

println a (b.x) (b 3) (b 4) (b.y.a) (b.y.b) : b.y.d 3 : b.y.c 6
