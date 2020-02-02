# Test emily.array module

p = package.emily.array

r = p.range 5 10

# Expect: 5. 6.
print (r.count) sp (r 1) ln

# Expect:
# 5.
# 6.
# 7.
# 8.
# 9.
r.each println

# Expect:
# 0.
# 1.
# 2.
# 3.
# 4.
r2 = p.rangeTo 5
r2.each println

# Expect:
# 5.
# 100.
# 7.
# 8.
# 9.
# 6.
r3 = p.copy r
r3 1 = 100
r3.each println
println: r 1

# Expect:
# <true>
# <null>
println: p.contains [1,2,3,4,5] 3
println: p.contains [1,2,8,4,5] 3
