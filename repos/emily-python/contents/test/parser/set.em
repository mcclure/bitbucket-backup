# Test = macro

# Arg: --ast2
# Expect: [Sequence(Scoped) [Set Scope [AtomLiteral a] [NumberLiteral 3]] [Let Scope [AtomLiteral b] [NumberLiteral 4]]]

a = 3
let b = 4