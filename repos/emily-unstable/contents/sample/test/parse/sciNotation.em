# Tests for the scientific notation and nondecimal float literals

# Expect: 0.
println 0e12
# Expect: 1.
println 1e-0
# Expect: 10.
println 1e+1

# Expect: <true>
println: 1E2 == 10E1
# Expect: <true>
println: 0.1E1 == 10E-1

# Alternate bases:

# Expect: 2721.
println 0xAa1
# Expect: 449.
println 0o701
# Expect: 181.
println 0b10110101