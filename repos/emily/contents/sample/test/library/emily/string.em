# Test emily.string module

p = package.emily.string
map = package.emily.functional.util.map

# Expect:
# a, b, c
println: p.join ", " ["a", "b", "c"]

# Expect:
# a⚧c
println: p.join "": map (p.fromCodepoint) [97, 9895, 99]