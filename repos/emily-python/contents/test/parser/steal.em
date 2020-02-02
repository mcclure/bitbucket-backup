# Test multi-line macros
# Arg: --ast2
# Expect: [Sequence(Scoped) [If [Var x] [Sequence(Returning) [NumberLiteral 3]] [Sequence(Returning) [NumberLiteral 4]]] [If [Var x] [Sequence(Returning) [NumberLiteral 5]] [Sequence(Returning) [NumberLiteral 6]]] [Apply [Apply [Var someFunction] [If [Var x] [Sequence(Returning) [NumberLiteral 7]] [Sequence(Returning) [NumberLiteral 8]]]] [NumberLiteral 9]] [Apply [Var someFunction] [If [Var x] [Sequence(Returning) [NumberLiteral 10]] [Sequence(Returning) [NumberLiteral 11]]]] [If [Var x] [Sequence(Returning) [NumberLiteral 12]] [Sequence(Returning) [NumberLiteral 13]]] [Array [If [Var x] [Sequence(Returning) [NumberLiteral 14]] [Sequence(Returning) [NumberLiteral 15]]]] [Apply [Var someFunction] [If [Var x] [Sequence(Returning) [NumberLiteral 16]] [Sequence(Returning) [NumberLiteral 17]]]] [Let Scope [AtomLiteral z] [If [Var x] [Sequence(Returning) [NumberLiteral 18]] [Sequence(Returning) [NumberLiteral 19]]]]]

if (x)
	3
else
	4

if (x)
	5


else
	6

someFunction
	if (x)
		7
	else
		8
	9

someFunction (
	if (x)
		10
	else
		11
)

(
	if (x)
		12
	else
		13
)

array
	if (x)
		14
	else
		15

# I'm not sure I like this, but currently it works
someFunction if (x)
	16
else
	17

let z = if (x)
	18
else
	19
