# Test export when nothing is exported but macros. IS ALSO IMPORTED BY MACRO.2.EM
# Arg: --exported
# Expect:
# [Macros: 1]

export macro
	splitMacro(600, "add")
