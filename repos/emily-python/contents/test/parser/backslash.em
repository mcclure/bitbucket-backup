# Test backslash parsing
# Arg: --ast2
# Expect: [Sequence [Set Scope [AtomLiteral let] [Var do]] [Apply [Var x] [AtomLiteral do]] [Apply [Var x] [AtomLiteral do]]]

\let = \do
x.do
x \.do
