# Test some arithmetic macros.
# Expect:
# 7.
# 12.
# 23.
# 4.
# -19.
# -3.

println( 3 + 4 )
println( 3 * 4 )
println( 3 + 4 * 5 )
println( 3 + 4 - 2 * 3 / 6 * 3 )
println( 3 + ~4 * 5 + 1 * ~2 )
println( ~3 * ~4 % ~5 )
