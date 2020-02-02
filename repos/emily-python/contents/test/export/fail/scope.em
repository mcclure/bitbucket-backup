# "export" is not allowed inside a scope.
# A subtle second test: "export" by itself elevates a sequence to scopehood.
# Expect failure

do
	export q = 3
