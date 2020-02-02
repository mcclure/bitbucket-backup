# Ensure tokens become execs correctly

# Arg: --ast2
# Expect: [Sequence [Apply [Apply [Apply [Apply [NumberLiteral 3] [StringLiteral "ok"]] [Var x]] [Var `]] [AtomLiteral x]]]

3.0 "ok" x ` .x