# One trailing comma at the end of a line may be forgiven

# Arg: --ast
# Expect: (a, a, a, b, b, b, , c, c, c, d (e, f), g)

# Both readers get confused by lines which end with a comma, but in different ways
# Tags: broken-all

a, a, a,
b, b, b,, # FIXME: Wait, this feels unexpected?
c, c
c,

d
	e,
	f,

g,
