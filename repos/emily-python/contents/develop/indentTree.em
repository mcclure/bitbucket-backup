# Takes input and turns () [] into indentation

let left = null
let right = null
let indentBy = "    "

with (stdin.peek) match
	"(" = do
		left = "("
		right = ")"
	"[" = do
		left = "["
		right = "]"
	"{" = do
		left = "{"
		right = "}"
	_ = do
		stderr.write
			"Error: First character of input is " 
			stdin.peek
			", but only ( [ { recognized"
			ln
		exit 1

let Linked = inherit Object
	field value = null
	field next = null
	method more = (!= (this.next) null)

let TreeNode = inherit Linked

# A lot of the logic here depends on the assumption, forced above, that the first char is a paren
let stack = null
let root = null
let current = ""
let failed = null

let flush = function()
	if (current.length)
		stack.value.append current
		current = ""

let push = function()
	flush()
	let node = new TreeNode(array(), stack)
	if (stack)
		stack.value.append node
	else
		root = node
	stack = node

let pop = function()
	flush()
	stack = stack.next

while (and (not failed) (stdin.more))
	let ch = stdin.next

	if (== ch right)
		if (stack)
			pop()
		else
			failed = "Too many right parenthesis, halting"
	elif (char.isSpace ch)
		flush()
	else
		if (and (root) (not stack))
			failed = "Extra junk after final closing parenthesis"
		else
			if (== ch left)
				push()
			else
				current = (+ current ch)

if (stack)
	failed = "Input ended but left parenthesis are still unclosed"

# Print the items of an array; indent items after first with "indent"
# This will crash if the first item of any array is itself an array
let printIndent = function (ary, indent)
	let i = ary.iter
	let indented = null

	while (i.more)
		if (indented)
			stdout.write ln indented
		else
			indented = (+ indent indentBy)

		let content = i.next
		with content match
			TreeNode innerAry = do
				printIndent (innerAry, indented)
			_ =
				stdout.write content

printIndent (root.value, "")
print ln

if (failed)
	stderr.write
		"Error: "
		failed
		ln
	exit 1
