# Demonstrate basic math
# Note: Same as macro/math/basic test

# Expect:
# <true>
# <null>
# <true>
# <null>
# <true>
# <null>

println ( ( 4 .minus 3 ) .lt ( 2 .plus 3 ) )      # 4-3 < 2+3 : 1 < 5 : true
println ( ( 2 .times 4 ) .gt ( 18 .divide 2 ) )   # 2*4 > 18/2 : 8 > 9 : null
println ( ( 4 .plus 4 ) .lte ( 16 .divide 2 ) )   # 2+4 <= 16/2 : 8 <= 8 : true
println ( ( 9 .divide 4 ) .gte ( 9 .minus 4 ) )   # 9/4 >= 9-4 : about 2 >= 5 : null
println ( ( 15 .minus 5 ) .eq  ( 20 .divide 2 ) ) # 15-5 == 20/2 : 10 == 10 : true
println ( ( 3 .times 4 ) .eq ( 3 .plus 4 ) )      # 3*4 == 3+4 : 12 == 7 : null