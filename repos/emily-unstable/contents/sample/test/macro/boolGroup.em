# Order of operation tests of boolean/logic operators

# Expect: <true>
println: 4 < 5

# Expect: <true>
println: 4 < 5 && 1 < 9

# Expect: <null>
println: () && 4

# Expect: 4.
println: 5 && 4

# Expect: <null>
println ( !(!5 || 4) )

# Test short circuit:

# Expect: 3.
println 3 || println 5

# Expect:
# 6.
# 8.
println 6 && println 8

# Print nothing at all!
3 || (println "o"; 4) ? 1 : 2