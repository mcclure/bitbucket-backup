# Demonstrate equality on non-number values

# Expect: <true>
println ( null .eq null )
# Expect: <true>
println ( true .eq true )
# Expect: <null>
println ( null .eq true )
# Expect: <null>
println ( true .eq null )

# Expect: <true>
println ( "ok" .eq "ok" )
# Expect: <null>
println ( "ok" .eq "bad" )
# Expect: <true>
println ( .ok  .eq .ok )
# Expect: <null>
println ( .ok  .eq .bad )
# Expect: <null>
println ( "ok" .eq .ok )

# Expect: <true>
println ( 400 .eq 4e2 )
# Expect: <true>
println ( 0.04 .eq 4.0E-2 )
# Expect: <null>
println ( 1.0 .eq 2.0e3 )

# Expect: <null>
println ( 2 .eq 3 )
# Expect: <true>
println ( 3 .eq 3 )

# Expect: <null>
println ( true .eq 3 )
# Expect: <null>
println ( null .eq .null ) # Notice the dot