# Test what () does

# Arg: --ast2
# Expect: [Sequence [Apply [Var a] [NullLiteral]] [Apply [NullLiteral] [Var b]]]

a ()
() b