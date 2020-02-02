# Verify set invocations don't bind "too early".
# Expect:
# 5.
# 3.

let .a 3
{
    let .aset (set .a)
    let .a 4
    aset 5
    println a
}
println a