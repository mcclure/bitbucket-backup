# Test using macro/profile on a package also imports symbols AND this works with "export profile"

profile project.implicitImportInclude

# Expect: false false -3

print
	1.0 && null
	\&& (1.0, null)
	\~ 3
	ln
