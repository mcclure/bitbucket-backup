# Test export. IS ALSO IMPORTED BY MACRO.EM
# Arg: --exported
# Expect:
# Printed
# [Macros: 1]
# w: 2

export w = 2

export macro
	splitMacro(600, "add")

macro
	splitMacro(610, "times")

println "Printed"
