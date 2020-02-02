# Reader: Turns unicode string iterator into parse tree

from core import *
from util import switch, dynamicSwitch, unicodeJoin, quotedString
import unicodedata

# AST

class ReaderException(EmilyException):
	pass

class ExpGroup(Node):
	def __init__(s, loc, openedWithParenthesis=False):
		super(ExpGroup, s).__init__(loc)
		s.openedWithParenthesis = openedWithParenthesis
		s.statements = []
		s.appendStatement()
		s.indent = None

	def finalStatement(s):
		return s.statements[-1]

	def nonempty(s):
		return len(s.statements) > 1 or (len(s.statements) == 1 and s.statements[0].nodes)

	def appendStatement(s):
		s.statements.append( Statement() )

	def __unicode__(s):
		return u"(%s)" % (unicodeJoin(u", ", s.statements))

class StringContentExp(Node):
	def __init__(s, loc):
		super(StringContentExp, s).__init__(loc)
		s.content = u''

	def append(s, ch):
		s.content += ch

class SymbolExp(StringContentExp):
	def __init__(s, loc, isAtom = False, isEscaped = False):
		super(SymbolExp, s).__init__(loc)
		s.isAtom = isAtom
		s.isEscaped = isEscaped

	def __unicode__(s):
		return (u"." if s.isAtom else u"") + s.content

class QuoteExp(StringContentExp):
	def __unicode__(s):
		return quotedString(s.content)

class NumberExp(Node):
	def __init__(s, loc):
		super(NumberExp, s).__init__(loc)
		s.integer = u''
		s.dot = False
		s.decimal = None

	def append(s, ch):
		if s.dot:
			if s.decimal is None:
				s.decimal = u''
			s.decimal += ch
		else:
			s.integer += ch

	def appendDot(s):
		if s.integer == u'':
			s.integer = '0'
		s.dot = True

	def __unicode__(s):
		return '#' + s.integer + (u"." if s.dot else u"") + (s.decimal if s.decimal is not None else u"")

class Statement(Printable): # Not a node, only a helper for ExpGroup
	def __init__(s):
		s.nodes = []
		s.dead = False # Wait, this isn't even used anywhere is it?

	def finalNode(s):
		return s.nodes[-1]

	def __unicode__(s):
		return unicodeJoin(u" ", s.nodes)

# Reader/Lexer

# Lexer FSM:
# All states except Quote,Comment,Dot,Cr,Indent:
#     ( -> [Push frame] Scanning; ) -> [Pop frame] [un-Dead] Scanning;
#     , -> [un-Dead] Scanning; \r -> Cr; LineSpace -> [newline] Indent; # -> Comment
# Indent:
#     NonLineSpace -> [Check indent level, push or pop frame];
#     Comment -> [Indent]; , -> [Error];
#     Other -> [Verify/set indent level unless line blank] Reinterpret as if scanning
# Scanning:
#     NonLineSpace -> Scanning; . -> Dot;
#	  Digit -> Number; " -> Quote; # -> Comment, Other -> Symbol
# Cr: \n -> [newline] Scanning; Other -> [newline] Reinterpret as if Scanning
# Dot: Digit -> Number; ()#".[newline] -> [Error,Dead]; Other -> Symbol
# Symbol:
#     Number, LineSpace, ()#." -> Reinterpret as if Scanning;
#     NonLineSpace -> Scanning; Other -> Symbol
# Number:
#     Digit -> Number; Dot -> Number; Dot[multiple] -> [Error] Dead;
#     NonLineSpace -> Scanning; Other -> Reinterpret as if Scanning
# Quote:
#     " -> Scanning; '\' -> [Backslashed]; [When backslashed] nrt\" -> [Reinterpret] Quote;
#     \r -> QuoteCr; [LineSpace] -> [Eat if backslashed] [newline] Quote;
#     '\other' -> [Error] Quote; Other -> Quote
# QuoteCr:
#     \n -> [Eat if backslashed] [newline] Quote; Other -> [newline] Reinterpret as if Quote
# Comment: \r \n -> [newline] Scanning; Other -> Comment

# Indent: Line-starting whitespace
# Cr: Hit CR, looking for LF
# Dot: Ambiguous; could become either Symbol or Number
# QuoteCr: Hit CR, looking for LF, inside quote
class ReaderState:
	Indent, Scanning, Cr, Dot, Backslash, Symbol, Number, Quote, QuoteCr, Comment = range(10)

def isNonLineSpace(ch):
	if ch == '\t':
		return True
	return unicodedata.category(ch) == 'Zs'
def isLineSpace(ch):
	if ch == '\r' or ch == '\n':
		return True
	cat = unicodedata.category(ch)
	return cat == 'Zl' or cat == 'Zp'
def isQuote(ch):
	if ch == u'"':
		return True
	cat = unicodedata.category(ch)
	return cat == 'Pi' or cat == 'Pf'
def isOpenParen(ch):
	return unicodedata.category(ch) == 'Ps'
def isCloseParen(ch):
	return unicodedata.category(ch) == 'Pe'
def isDigit(ch):
	return ord(ch) >= ord(u'0') and ord(ch) <= ord(u'9')
def isIdStart(ch): # FIXME: FOLLOW UAX 31
	return unicodedata.category(ch)[0] == 'L' or ch=='_'
def isIdContinue(ch):
	return isIdStart(ch) or isDigit(ch)

class Reader:
	def __init__(s, filetag = None):
		s.filetag = filetag
		s.line = 1
		s.char = 0
		s.groupStack = []
		s.errors = []
		s.appendGroup()

	def loc(s):
		return Loc(s.filetag, s.line, s.char)

	def finalGroup(s):
		return s.groupStack[-1]

	def finalExp(s):
		return s.finalGroup().finalStatement().finalNode()

	def appendExp(s, exp):
		s.finalGroup().finalStatement().nodes.append( exp )

	def appendGroup(s, inStatement = False, openedWithParenthesis = False):
		group = ExpGroup(s.loc(), openedWithParenthesis)
		if inStatement:
			s.appendExp( group )
		s.groupStack.append( group )

	def reset(s, state, backslashed = False):
		s.parserState = state
		s.backslashed = backslashed
		s.identifier = None
		s.currentIndent = u''
		for case in switch(state):
			if case(ReaderState.Number):
				s.appendExp( NumberExp(s.loc()) )
			elif case(ReaderState.Symbol):
				s.appendExp( SymbolExp(s.loc()) )
			elif case(ReaderState.Quote):
				s.appendExp( QuoteExp(s.loc()) )

	def newline(s):
		s.line += 1
		s.char = 0

	def unrollTo(s, unrollIdx):
		while len(s.groupStack) > unrollIdx:
			s.groupStack.pop()

	def handleLineSpace(s, ch):
		if ch == '\r':
			s.reset(ReaderState.Cr)
		else:
			s.newline()
			s.reset(ReaderState.Indent)

	def error(s, msg, survivable = False): # Survivable as in: Can we continue parsing syntax
		s.errors.append(Error(s.loc(), msg))
		if not survivable:
			s.finalGroup().finalStatement().dead = True
			s.reset(ReaderState.Scanning)

	def ast(s, iter):
		s.reset(ReaderState.Indent)

		for ch in iter:
			for case in dynamicSwitch(s, 'parserState'):
				s.char += 1

				if case(ReaderState.Cr):
					s.newline()
					s.newexp()
					s.reset(ReaderState.Indent)
					if i == u'\n':
						break # If complete CRLF, DONE

				if case(ReaderState.Indent) and not isCloseParen(ch):
					if isNonLineSpace(ch):
						s.currentIndent += ch
						break # If indent continued, DONE
					if isLineSpace(ch):
						s.handleLineSpace(ch)
						break # If indent ended line (with newline), DONE
					if ch == u'#':
						s.reset(ReaderState.Comment)
						break; # If indent ended line (with comment), DONE
					if ch == u',':
						s.error("Comma at start of line not understood") # FIXME: This should clear entire line
						break # If line starts with comment (illegal), DONE

					# Line has begun! Adjust group and move into scanning state:
					finalGroup = s.finalGroup()

					# If group already contains lines
					if finalGroup.indent == s.currentIndent:
						if finalGroup.finalStatement().nodes:
							finalGroup.appendStatement()

					# First line of group
					if finalGroup.indent is None:
						finalGroup.indent = s.currentIndent

						if finalGroup.finalStatement().nodes:
							finalGroup.appendStatement()
							# FIXME: Is this really the right thing to do?
							s.error("Indentation after ( is ambiguous; please add a , at the end of the previous line.")

					# Indent or dedent event
					elif finalGroup.indent != s.currentIndent:
						# Indent event
						if s.currentIndent.startswith(finalGroup.indent):
							s.appendGroup(True)
							s.finalGroup().indent = s.currentIndent

						# Dedent event (or error)
						else:
							unrollIdx = len(s.groupStack) - 1
							parenthesisIssue = s.finalGroup().openedWithParenthesis
							if not parenthesisIssue:
								while True:
									unrollIdx -= 1
									if unrollIdx < 0:
										break
									group = s.groupStack[unrollIdx]
									if group.indent == s.currentIndent:
										break
									if group.openedWithParenthesis:
										parenthesisIssue = True
										break

							# Error
							if parenthesisIssue:
								group = s.groupStack[unrollIdx] # Redundant but ehhh
								s.error("Indentation on this line doesn't match any since parenthesis on line %d char %d" % (group.loc.line, group.loc.char))
							elif unrollIdx < 0:
								s.error("Indentation on this line doesn't match any previous one")

							# Dedent
							else:
								s.unrollTo(unrollIdx+1)
								s.finalGroup().appendStatement()

					# If we didn't break above, we're done with the indent.
					s.reset(ReaderState.Scanning)

				if case(ReaderState.Dot):
					if isNonLineSpace(ch):
						break # Whitespace after dot is not interesting. DONE
					elif isDigit(ch):
						s.reset(ReaderState.Number)
						s.finalExp().appendDot()
					elif ch == u'.' or ch == u'(' or ch == u')' or ch == u'"' or ch == '\\':
						s.error("'.' was followed by special character '%s'" % ch)
					elif ch == u'#' or isLineSpace(ch):
						s.error("Line ended with a '.'")
					else:
						s.reset(ReaderState.Symbol)
						s.finalExp().isAtom = True

				if case(ReaderState.Quote):
					trueCh = ch
					if s.backslashed:
						for chCase in switch(ch):
							if chCase(u'\\'):
								trueCh = u'\\'
							elif chCase('n'):
								trueCh = u'\n'
							elif chCase('r'):
								trueCh = u'\r'
							elif chCase('t'):
								trueCh = u'\t'
							elif isQuote(ch):
								pass # trueCh is already ch
							else:
								trueCh = None

						s.backslashed = False

						if trueCh is None:
							s.error("Unrecognized backslash sequence '\%s'" % (ch), True)
							break # Don't know what to do with this backslash, so just eat it. DONE
					elif ch == u'\\':
						s.backslashed = True
						break # Consumed backslash inside string. DONE
					elif isQuote(ch):
						s.reset(ReaderState.Scanning)
						break # Consumed quote at end of string. DONE.

					s.finalExp().append(trueCh)
					break

				# These checks are shared by: Scanning Symbol Number Comment
				if isLineSpace(ch):
					s.handleLineSpace(ch)
					break # Consumed newline. DONE

				if case(ReaderState.Comment): # FIXME: This + linespace could go earlier?
					break # Inside comment, don't care. DONE

				# These checks are shared by: Scanning Symbol Number (and Indent for right parens)
				if isNonLineSpace(ch):
					s.reset(ReaderState.Scanning)
					break
				if isOpenParen(ch):
					s.appendGroup(True, True)
					s.reset(ReaderState.Scanning)
					break # Have consumed (. DONE
				if isCloseParen(ch):
					# Unroll stack looking for last parenthesis-based group
					unrollIdx = len(s.groupStack)
					while True:
						unrollIdx -= 1
						if unrollIdx < 0:
							break
						group = s.groupStack[unrollIdx]
						if group.openedWithParenthesis:
							break

					# None found
					if unrollIdx < 0:
						s.error("Stray right parenthesis matches nothing", True)
					else:
						s.unrollTo(unrollIdx)

					s.reset(ReaderState.Scanning)
					break # Have consumed (. DONE
				if isQuote(ch):
					s.reset(ReaderState.Quote)
					break # Have consumed ". DONE
				if ch == u',':
					s.finalGroup().appendStatement()
					s.reset(ReaderState.Scanning)
					break # Have consumed ,. DONE
				if ch == u'#':
					s.reset(ReaderState.Comment)
					break # Have consumed #. DONE

				if ch == u'\\': # Shared by: Scanning Symbol Number
					s.reset(ReaderState.Backslash)
				elif ch == u'.': # Shared by: Scanning Symbol Number
					if case(ReaderState.Number):
						if s.finalExp().dot:
							s.reset(ReaderState.Dot) # We're starting an atom or something
						else:
							# FIXME: This will ridiculously require 3..function to call a function on number
							s.finalExp().dot = True # This is a decimal in a number
					elif case(ReaderState.Backslash):
						s.reset(ReaderState.Symbol)
						s.finalExp().isAtom = True
						s.finalExp().isEscaped = True
					else:
						s.reset(ReaderState.Dot)
					break # Have consumed .. DONE
				elif isDigit(ch):
					if case(ReaderState.Backslash):
						s.error(u"Backslash cannot be followed by a number")
					elif case(ReaderState.Scanning) or (case(ReaderState.Symbol) and s.identifier is False):
						s.reset(ReaderState.Number)
				else: # Symbol character
					if case(ReaderState.Scanning) or case(ReaderState.Number):
						s.reset(ReaderState.Symbol)
					elif case(ReaderState.Backslash):
						s.reset(ReaderState.Symbol)
						s.finalExp().isEscaped = True

				if case(ReaderState.Number):
					s.finalExp().append(ch)
					break # Added to number. DONE

				if case(ReaderState.Symbol):
					if s.identifier is None:
						s.identifier = isIdStart(ch)
					else:
						if (s.identifier and not isIdContinue(ch)) or (not s.identifier and isIdStart(ch)):
							s.reset(ReaderState.Symbol)
							s.identifier = isIdStart(ch)
					s.finalExp().append(ch)
					break # Added to symbol. DONE

		# Done; unroll all groups
		while len(s.groupStack) > 1:
			group = s.finalGroup()
			if group.openedWithParenthesis:
				s.error("Parenthesis on line %d char %d never closed" % (group.loc.line, group.loc.char))
			s.groupStack.pop()

def ast(iter, filetag = None):
	parser = Reader(filetag)
	parser.ast(iter)
	if parser.errors:
		output = [errorFormat(e) for e in parser.errors]
		raise ReaderException(u"\n".join(output))
	result = parser.finalGroup()
	if not result.statements[0].nodes: # Special case: Empty file produces bad AST
		result.statements = []
	return parser.finalGroup()

