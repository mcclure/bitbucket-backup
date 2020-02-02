# Test boolean/logic operators
# Expect:
# <null>
# <null>
# <null>
# <true>
# <null>
# <true>
# <true>
# <true>
# <null>
# <true>
# <true>
# <null>

println ( null && null )
println ( null && true )
println ( true && null )
println ( true && true )
println ( null || null )
println ( null || true )
println ( true || null )
println ( true || true )
println ( null %% null )
println ( null %% true )
println ( true %% null )
println ( true %% true )
