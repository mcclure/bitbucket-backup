# Test a has statement.
# Expect:
# <true>
# <null>
# <true>

# Test has on a builtin
println (has .println)

# Test has on a user defined value
println (has .a)
let .a 3
println (has .a)