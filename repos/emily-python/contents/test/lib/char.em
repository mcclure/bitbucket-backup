# String garbage
# TODO: Test some unicode

# Expect:
# false false false true true false false false
# false false true false false false false false
# false false true true true false false false
# false false false false false false false true
# false false false false false true false false
# false false false false false false true false
# false false false false false true true false
# false true false false false false false false

let testChars = array
	"x"
	"3"
	"\n"
	" "
	"\t"
	"("
	"]"
	"\""

let testFunctions = array
	.isNonLineSpace
	.isLineSpace
	.isSpace
	.isQuote
	.isOpenParen
	.isCloseParen
	.isParen
	.isDigit

let iFn = testFunctions.iter
while (iFn.more)
	let fn = iFn.next

	let iCh = testChars.iter
	while (iCh.more)
		print ((char fn) (iCh.next))

	print ln
