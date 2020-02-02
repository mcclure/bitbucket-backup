# Test when sequences do or don't get the scoped flag
# Note: This feature tests an optimization, and the effect isn't user-noticeable unless you use --ast2.
# Worse, I don't intend to do it this way indefinitely. I am ambivalent about including this test.

# Arg: --ast2

# Expect: [Sequence [Sequence(Scoped, Returning) [Let Scope [AtomLiteral x] [NumberLiteral 3]]] [Sequence(Returning) [Set Scope [AtomLiteral y] [NumberLiteral 4]]] [Sequence(Scoped, Returning) [Export Scope [AtomLiteral z] [NumberLiteral 5]]]]

do
	let x = 3

do
	y = 4

do
	# Note: This is illegal, but (for now) it's allowed up until the parse stage.
	export z = 5
