# Sister of fail/ambiguousIndent, but with an added comma to make it non-ambiguous

# Arg: --ast
# Expect: (a b (c, d, e), f)

a b (c,
	d
	e)
f