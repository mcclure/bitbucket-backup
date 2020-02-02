# Test importing macros
# Expect:
# Printed
# 7

let add = \+

# Horrifyingly, this will have the side effect of printing "Printed", during compile
macro project.customMacro

println
	3 add 4
