# Shared classes used by all bits of the interpreter chain

profile experimental

from project.util import *

# TODO: Important question to consider:
# Is it actually useful to have macro stages fit "between" parser and executable? 
# I think I originally imagined macros would update nodes to "at least" their own
# progress level, but in practice it seems macros only update targets all the way to
# "executable". Maybe macro-priority and progress should be two separate enums
export ProgressBase = inherit Object
	none = 0   
	reader = 1000      # Expression tree structure
	parser = 2000      # "AST" structure
	executable = 3000  # Execution tree structure

export Loc = inherit Object
	field file = null
	field line = 0
	field char = 0

	method toString = nullJoin array
		if (this.file)
			this.file + ", "
		else
			""
		"line "
		this.line
		" char "
		this.char

export Node = inherit Object
	field loc = null
	progress = ProgressBase.none

	method toString = "[Node " + this.loc.toString + "]"

export Error = inherit Object
	field loc = null
	field msg = null
