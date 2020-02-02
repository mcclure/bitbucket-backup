# Test a return statement.
# Expect:
# 3.
# 4.

a ^ = return 3

b ^ = {
    c ^arg = arg 4
    c return
    return 5
}

println: do a
println: do b
