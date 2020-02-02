# Basic math test with macros
# Note: Same as math/basic test

# Expect:
# <true>
# <null>
# <true>
# <null>
# <true>
# <null>

println: 4 - 3 < 2 + 3     # 4-3 < 2+3 : 1 < 5 : true
println: 2 * 4 > 18 / 2    # 2*4 > 18/2 : 8 > 9 : null
println: 4 + 4 <= 16 / 2   # 2+4 <= 16/2 : 8 <= 8 : true
println: 9 / 4 >= 9 - 4    # 9/4 >= 9-4 : about 2 >= 5 : null
println: 15 - 5 ==  20 / 2 # 15-5 == 20/2 : 10 == 10 : true
println: 3 * 4  ==  3 + 4  # 3*4 == 3+4 : 12 == 7 : null