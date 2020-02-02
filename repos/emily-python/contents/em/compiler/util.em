profile experimental

from project.util import *

# Helpers

export indentPrefix = function (count)
	let prefix = ""
	while (count > 0)
		prefix = prefix + "    "
		count = count - 1
	prefix

export indent = function (count)
	let prefix = indentPrefix count
	map function(s)
		prefix + (s.toString)

export arrayIndented = function (a, i) (join "\n" (indent i a))

export NumGenerator = inherit Object
	field generator = 0 - 1

	method next = do
		this.generator = this.generator + 1
		this.generator
