# Test that non-explicit closures don't get their own returns.
# Expect:
# 5.
# 20.
# 32.

# Return inside ?: implicit closure
a ^b = (
    b > 5 ? return 5 : null
    return b
)

# Return inside ^ construct
c ^d = {
    ret = ^ x ( return x )
    ret 20
    return d
}

# Return inside ^@ (with return) construct
e ^f = {
    ret = ^@ x ( return x )
    ret 20
    return f
}

println: a 10
println: c 21
println: e 32