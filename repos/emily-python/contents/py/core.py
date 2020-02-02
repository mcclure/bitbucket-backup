# Shared classes used by all bits of the interpreter chain

# Every Node has a progress field. The progress of a node >= the progress of its children (if any)

# Progress comes in several phases:
class ProgressBase:
	Reader = 1000      # Expression tree structure
	Parser = 2000     # "AST" structure
	Executable = 3000  # Execution tree structure

class EmilyException(Exception):
	pass

class Printable(object):
	def __str__(s):
		return unicode(s).encode('utf-8')

class Node(Printable):
	# Currently the reader handles ., comma and () itself so lowest possible is 1000
	def __init__(s, loc, progress = ProgressBase.Reader):
		s.loc = loc
		s.progress = progress

class Error(object):
	def __init__(s, loc, msg):
		s.loc = loc
		s.msg = msg

class Loc(Printable):
	def __init__(s, file, line, char):
		s.file = file
		s.line = line
		s.char = char

def errorFormat(e):
	return u"%s%sine %s char %s: %s" % (e.loc.file if e.loc.file else "", ": l" if e.loc.file else "L", e.loc.line, e.loc.char, e.msg)
