# Parser: Transformations ("macros") applied to parse tree to gradually make it executable

from core import *
from util import switch
import reader
import execution

class ParserException(EmilyException):
	pass

# Standard macros-- "ast"

class Macro(object):
	def __init__(s, progress):
		s.progress = progress

# Macro machinery

class MacroLevel(object):
	def __init__(s, progress, contents):
		s.progress = progress
		s.contents = contents

class MacroShortCircuit(Exception):
	def __init__(s, error):
		s.error = error

class SequenceTracker(object):
	def __init__(s, statements):
		s.statements = statements
		s.idx = 0
		s.argIdx = 0

	def steal(s, symbol):
		if s.more():
			nodes = s.statements[s.idx].nodes
			if nodes and isSymbol(nodes[0], symbol):
				s.idx += 1
				return nodes
		return None

	def __iter__(s):
		return s

	def more(s):
		return s.idx < len(s.statements)

	def next(s):
		if s.more():
			statement = s.statements[s.idx]
			s.idx += 1
			s.argIdx += 1
			return statement
		else:
			raise StopIteration()   	

class BidiIterator(object):
	def __init__(s, source, rightward):
		if rightward:
			s.left = source
			s.right = []
		else:
			s.left = []
			s.right = source
		s.rightward = rightward

	def source(s):
		return s.left if s.rightward else s.right

	def result(s):
		return s.right if s.rightward else s.left

	def push(s, v):
		if s.rightward:
			s.right.insert(0, v)
		else:
			s.left.append(v)

	def pop(s):
		if s.rightward:
			return s.left.pop()
		else:
			return s.right.pop(0)

	def replace(s, left, at, right):
		s.left = left or []
		s.right = right or []
		if at:
			s.push(at)

class Parser(object):
	def __init__(s, clone = None, clear = False):
		s.errors = clone.errors if clone else []
		if clear:
			s.macros = []
		else:
			if clone:
				s.macros = [MacroLevel(level.progress, list(level.contents)) for level in clone.macros]
			else:
				s.macros = []
				s.loadAll(defaultMacros)

	def _innerLoad(s, macro):
		for existing in s.macros:
			if existing.progress == macro.progress:
				existing.contents.append(macro)
				return
		s.macros.append(MacroLevel(macro.progress, [macro]))

	def _sort(s):
		s.macros.sort(key=lambda x:x.progress)

	def load(s, macro):
		s._innerLoad(macro)
		s._sort()

	def loadAll(s, macros): # FIXME: O(n^2)
		for macro in macros:
			s._innerLoad(macro)
		s._sort()

	def checkComplete(s, node):
		if node.progress < ProgressBase.Executable:
			return s.errorAt(node, "Internal error: Macro processing completed but this node is unfinished")
		return None

	def errorAt(s, loc, msg):
		s.errors.append( Error(loc, msg) )
		return execution.InvalidExec(loc)

	def process(s, nodes, tracker = None):
		if not nodes:
			raise Exception("Internal error: macro process() cannot work on an empty list")
		try:
			for level in s.macros:
				i = BidiIterator(nodes, 1 == level.progress % 2)
				while i.source():
					at = i.pop()
					left = i.left
					right = i.right
					if at.progress > level.progress:
						i.push(at)
						continue
					for macro in level.contents:
						if macro.match(left, at, right):
							result = macro.apply(s, left, at, right, tracker)
							# TODO: Catch exceptions
							if type(result) == Error:
								raise MacroShortCircuit(result)
							else:
								(left, at, right) = result
							break
					i.replace(left, at, right)
				nodes = i.result()
		except MacroShortCircuit as e:
			return s.errorAt( e.error.loc, e.error.msg )

		if not nodes:
			# at is known non-None because otherwise we would have failed earlier
			return s.errorAt(at.loc, "Macro malfunctioned and produced an empty list")
		if type(nodes[0]) == reader.ExpGroup:
			if not nodes[0].nonempty():
				result = s.makeUnit(nodes.pop(0))
			else:
				nodes0 = nodes.pop(0)
				tracker = SequenceTracker(nodes0.statements)
				result = s.process(next(tracker).nodes, tracker)
				if tracker.more(): # Wait. There's more?
					return s.errorAt(nodes0.loc, "Line started with a multiline parenthesis group. Did you mean to use \"do\"?")
				nodes0 = None
		else:
			result = nodes.pop(0)
			completenessError = s.checkComplete(result)
			if completenessError:
				return completenessError

		while nodes:
			arg = nodes.pop(0)
			if type(arg) == reader.ExpGroup:
				if not arg.nonempty():
					result = execution.ApplyExec(result.loc, result, s.makeUnit(arg))
				else:
					tracker = SequenceTracker(arg.statements)
					for statement in tracker:
						if not statement.nodes:
							return s.errorAt(arg.loc, "Argument #%s to function is blank" % (tracker.argIdx))
						result = execution.ApplyExec(result.loc, result, s.process(statement.nodes, tracker))
			else:
				completenessError = s.checkComplete(arg)
				if completenessError:
					return completenessError
				result = execution.ApplyExec(result.loc, result, arg)

		s.checkComplete(result)
		return result

	def makeUnit(s, grp):
		return execution.NullLiteralExec(grp.loc)

	def makeSequence(s, loc, statements, shouldReturn = False):
		execs = []
		macros = []
		m = None
		tracker = SequenceTracker(statements)
		for stm in tracker:
			currentParser = m or s
			exe = currentParser.process(stm.nodes, tracker)
			if type(exe) == UserMacroList: # Apply these macros to all following lines
				if not m or exe.profile: # Instantiate parser on first custom macro, or any "profile" call
					m = Parser(currentParser, exe.profile) # Inherit from existing parser, clear if this is a "profile"
				m.loadAll(exe.contents)
				if exe.export:
					macros += exe.contents # FIXME: This endures even after a "profile". Is that weird?
				if exe.payload:
					execs.append(exe.payload)
			else:
				execs.append(exe)

		hasLets = bool(macros)
		if not hasLets:
			for exe in execs: # FIXME: This approach will do something weird if you = in a argument list or condition
				if type(exe) == execution.SetExec and (exe.isLet or exe.isExport):
					hasLets = True
					break

		return execution.SequenceExec(loc, shouldReturn, hasLets, execs, macros)

	def makeArray(s, seq):
		tracker = SequenceTracker(seq.statements)
		return [s.process(stm.nodes, tracker) for stm in tracker] if seq.nonempty() else []

# Standard macros-- "make values"

# TODO: do, if, while

def isSymbol(exp, match):
	return type(exp) == reader.SymbolExp and not exp.isAtom and not exp.isEscaped and exp.content == match

# Abstract macro: matches on just one known symbol
class OneSymbolMacro(Macro):
	def match(s, left, node, right):
		return isSymbol(node, s.symbol())

# Macro for loading macros -- Can masquerade as an Executable
class UserMacroList(execution.Executable):
	def __init__(s, loc, contents, export, profile, payload = None):
		super(UserMacroList, s).__init__(loc)
		s.contents = contents
		s.export = export
		s.profile = profile
		s.payload = payload # Executable to run after

	def __unicode__(s):
		return u"[Misplaced macro node]"

	def eval(s, scope):
		raise Exception("\"Macro\" statement in invalid place")

class MacroMacro(OneSymbolMacro):
	def __init__(s, profile):
		super(MacroMacro, s).__init__(progress=ProgressBase.Parser + 10)
		s.profile = profile

	def symbol(s):
		return u"profile" if s.profile else u"macro"

	def apply(s, m, left, node, right, _):
		export = False
		if left:
			if not (len(left) == 1 and isSymbol(left[0], "export")):
				return Error(node.loc, "Stray garbage before \"%s\"" % s.symbol())
			export = True
		if not right:
			return Error(node.loc, u"Emptiness after \"%s\"" % s.symbol())
		macroGroup = right[0]
		if type(macroGroup) == reader.SymbolExp:
			# TODO: Consider only allowing this if atom keys. TODO: Do something more sensible when this fails?
			macroObject = m.process(right).eval(execution.profileScope)
			try:
				macroList = macroObject.apply(execution.AtomLiteralExec(node.loc, execution.macroExportList))
			except execution.InternalExecutionException as e:
				raise execution.ExecutionException(macroGroup.loc, u"macro load", unicode(e))
			if type(macroList) != list:
				return Error(macroGroup.loc, u"macro import path did not resolve to a valid module")
			else:
				payload = None
				importObject = macroObject
				if isinstance(importObject, execution.LazyMacroLoader): # TODO: What about module objects?
					importObject = importObject.importObject()
				if importObject:
					payload = execution.ImportAllExec(node.loc, 
						execution.StoredLiteralExec(node.loc, importObject),
					export)
				return ([], UserMacroList(node.loc, macroList, export, s.profile, payload), [])
		elif type(macroGroup) == reader.ExpGroup:
			if len(right) > 1:
				return Error(node.loc, u"Stray garbage after \"%s (group)\"" % s.symbol())
			macros = m.makeArray(macroGroup)
			return ([], UserMacroList(node.loc, [ast.eval(execution.defaultScope) for ast in macros], export, s.profile), [])
		else:
			return Error(node.loc, u"Expected a path or a (group) after \"%s\"" % s.symbol())

class ImportMacro(OneSymbolMacro):
	def __init__(s):
		super(ImportMacro, s).__init__(progress=ProgressBase.Parser + 10)

	def symbol(s):
		return u"import"

	# FIXME: This entire setup too easily accepts nonsense like "import a + b from c + d"
	def generateSetExec(s, m, loc, prefix, target):
		if not target:
			return Error(loc, u"Missing target to import")
		if prefix:
			if type(target[0]) == reader.SymbolExp:
				if target[0].isAtom: # FIXME: This check should occur even if prefix does not
					return Error(target[0].loc, u"Expected a symbol after \"import\"")
				newSymbol = reader.SymbolExp(target[0].loc, True)
				newSymbol.content = target[0].content
				target = [newSymbol] + target[1:]
			target = prefix + target

		if len(target) == 1:
			return Error(target[0].loc, u"import expects either multiple symbols or a \"from\" clause")

		if type(target[-1]) != reader.SymbolExp or not target[-1].isAtom:
			return Error(target[-1].loc, u"End of import path needs to be an atom")

		symbol = execution.AtomLiteralExec(target[-1].loc, target[-1].content)

		return execution.SetExec(loc, True, False, False, False, None, symbol, m.process(target))

	def apply(s, m, left, node, right, tracker):
		prefix = None
		if left:
			if isSymbol(left[0], u"from"):
				if len(left) == 1:
					return Error(left[0].loc, u"Expected symbols between \"from\" and \"import\"")
				prefix = left[1:]
			else:
				return Error(node.loc, u"Stray garbage before \"import\"")

		if len(right) == 1 and type(right[0]) == reader.ExpGroup:
			setExecs = []
			for stm in right[0].statements:
				setExec = s.generateSetExec(m, node.loc, prefix, stm.nodes)
				if type(setExec) == Error:
					return setExec
				setExecs.append(setExec)
			result = execution.SequenceExec(node.loc, False, False, setExecs)
		elif len(right) == 1 and isSymbol(right[0], "*"):
			result = execution.ImportAllExec(node.loc, m.process(prefix))
		else:
			result = s.generateSetExec(m, node.loc, prefix, right)

		if type(result) == Error:
			return result

		return ([], result, [])

# = sign
class SetMacro(OneSymbolMacro):
	def __init__(s):
		super(SetMacro, s).__init__(progress = ProgressBase.Parser + 100)

	def symbol(s):
		return u"="

	def apply(s, m, left, node, right, tracker):
		isLet = False
		isMethod = False
		isField = False
		isExport = False
		target = None
		for idx in range(len(left)):
			if isSymbol(left[idx], u"let"):
				isLet = True
			elif isSymbol(left[idx], u"method"):
				isMethod = True
			elif isSymbol(left[idx], u"field"):
				isField = True
			elif isSymbol(left[idx], u"export"):
				isExport = True
			else:
				break
		if isLet and isExport:
			return Error(node.loc, "Cannot use \"let\" and \"export\" together")
		if left:
			left = left[idx:]
		if len(left) == 0:
			return Error(node.loc, "Missing name in =")
		key = left[-1]
		if len(left) > 1:
			target = m.process(left[:-1])
			key = m.process([key])
		else: # Currently under all circumstances a = b is a flat atom assignment
			if type(key) != reader.SymbolExp or key.isAtom:
				return Error(key.loc, "Assigned name must be alphanumeric")
			key = execution.AtomLiteralExec(key.loc, key.content)
		value = m.process(right, tracker)
		return ([], execution.SetExec(node.loc, isLet, isMethod, isField, isExport, target, key, value), [])

# Abstract macro: Expects SYMBOL (GROUP)
class SeqMacro(OneSymbolMacro):
	def apply(s, m, left, node, right, _):
		if not right:
			return Error(node.loc, u"Emptiness after \"%s\"" % (s.symbol()))
		seq = right.pop(0)
		if type(seq) != reader.ExpGroup:
			return Error(node.loc, u"Expected a (group) after \"%s\"" % (s.symbol()))
		return (left, s.construct(m, seq), right)

# do (statements)
class DoMacro(SeqMacro):
	def __init__(s):
		super(DoMacro, s).__init__(progress = ProgressBase.Parser + 400)

	def symbol(s):
		return u"do"

	def construct(s, m, seq):
		return m.makeSequence(seq.loc, seq.statements, True)

# if (cond) (ifBlock) (elseBlock?) -- OR -- while (cond) (whileBlock)
class IfMacro(OneSymbolMacro):
	def __init__(s, loop):
		super(IfMacro, s).__init__(progress = ProgressBase.Parser + 400)
		s.loop = loop

	def symbol(s):
		return u"while" if s.loop else u"if"

	def apply(s, m, left, node, right, tracker):
		if not right:
			return Error(node.loc, u"Emptiness after \"%s\"" % (node.content))
		cond = right.pop(0)
		if not right:
			return Error(node.loc, u"Emptiness after \"%s (condition)\"" % (node.content))
		seq = right.pop(0)
		if type(seq) != reader.ExpGroup:
			return Error(node.loc, u"Expected a (group) after \"%s (condition)\"" % (node.content))

		cond = m.process([cond])
		seq = m.makeSequence(seq.loc, seq.statements, not s.loop)

		elseq = None
		if not s.loop:
			if not right and tracker:
				right = tracker.steal(u"else")
			if not right and tracker:
				right = tracker.steal(u"elif")
			if right:
				if isSymbol(right[0], "else"):
					right.pop(0) # Throw away else symbol
					elseq = right.pop(0)
					if type(elseq) != reader.ExpGroup:
						return Error(node.loc, u"Expected a (group) after \"else\"")
					elseq = m.makeSequence(elseq.loc, elseq.statements, True)
				elif isSymbol(right[0], "elif"):
					elifSymbol = right.pop(0)
					_, elseq, right = s.apply(m, [], elifSymbol, right, tracker)

		return (left, execution.IfExec(node.loc, s.loop, cond, seq, elseq), right)

# function (args) (body) -- OR -- func (args) (body)
class FunctionMacro(OneSymbolMacro):
	def __init__(s):
		super(FunctionMacro, s).__init__(progress = ProgressBase.Parser + 400)

	def symbol(s):
		return u"function"

	def apply(s, m, left, node, right, _):
		name = node.content
		if not right:
			return Error(node.loc, u"Emptiness after \"%s\"" % (name))
		argSymbols = right.pop(0)
		if type(argSymbols) != reader.ExpGroup:
			return Error(node.loc, u"Expected a (group) after \"%s\"" % (name))
		if not right:
			return Error(node.loc, u"Emptiness after \"%s (args)\"" % (name))
		seq = right.pop(0)
		if type(seq) != reader.ExpGroup:
			return Error(node.loc, u"Expected a (group) after \"%s (args)\"" % (name))
		args = []
		if argSymbols.nonempty():
			for stm in argSymbols.statements:
				if not stm.nodes:
					return Error(node.loc, u"Arg #%d on %s is blank" % (len(args)+1, name))
				if type(stm.nodes[0]) != reader.SymbolExp:
					return Error(node.loc, u"Arg #%d on %s is not a symbol" % (len(args)+1, name))
				args.append(stm.nodes[0].content)
		return (left, execution.MakeFuncExec(node.loc, args, m.makeSequence(seq.loc, seq.statements, True)), right)

# match (matchbody)
class MatchCase(object):
	def __init__(s, targetExe, unpacks, statement):
		s.targetExe = targetExe
		s.unpacks = unpacks
		s.statement = statement

class MatchMacro(OneSymbolMacro):
	def __init__(s):
		super(MatchMacro, s).__init__(progress = ProgressBase.Parser + 400)

	def symbol(s):
		return u"match"

	def apply(s, m, left, node, right, tracker):
		if not right:
			return Error(node.loc, u"Emptiness after \"match\"")
		lines = right.pop(0)
		if type(lines) != reader.ExpGroup:
			return Error(node.loc, u"Expected a (group) after \"match\"")
		result = []
		for stmIdx in range(len(lines.statements) if lines.statements else 0):
			stm = lines.statements[stmIdx]
			if not stm.nodes: # Match is "like code" so may contain a blank line...?
				continue
			eqIdx = None # Find = sign
			for idx in range(len(stm.nodes)):
				if isSymbol(stm.nodes[idx], '='):
					eqIdx = idx
					break
			if eqIdx is None:
				return Error(node.loc, u"match line #%d does not have an =" % (stmIdx+1))
			eqNode = stm.nodes[eqIdx]
			eqLeft = stm.nodes[:eqIdx]
			eqRight = stm.nodes[eqIdx+1:]
			if not eqLeft:
				return Error(node.loc, u"On match line #%d, left of = is blank" % (stmIdx+1))
			if len(eqLeft) > 2:
				return Error(node.loc, u"On match line #%d, left of = has too many symbols. Try adding parenthesis?" % (stmIdx+1))
			if not eqRight:
				return Error(node.loc, u"On match line #%d, right of = is blank" % (stmIdx+1))
			target = eqLeft.pop(0)
			unpacksExp = None
			unpacks = []
			if eqLeft:
				unpacksExp = eqLeft[0]
				foundUnpack = False
				if type(unpacksExp) == reader.SymbolExp:
					unpacks = [execution.AtomLiteralExec(unpacksExp.loc, unpacksExp.content)]
					foundUnpack = True
				elif type(unpacksExp) == reader.ExpGroup:
					for statement in unpacksExp.statements:
						if not statement.nodes or type(statement.nodes[0]) != reader.SymbolExp:
							foundUnpack = False
							break
						unpacks.append(execution.AtomLiteralExec(statement.nodes[0].loc, statement.nodes[0].content))
						foundUnpack = True
				if not foundUnpack:
					return Error(node.loc, u"On match line #%d, variable unpack list on left of = is garbled" % (stmIdx+1))
			if isSymbol(target, '_'):
				if unpacksExp:
					return Error(node.loc, u"On match line #%d, variable unpack list used with _" % (stmIdx+1))
				target = None
			elif isSymbol(target, 'array'):
				if not unpacksExp:
					return Error(node.loc, u"On match line #%d, \"array\" used but no unpack list found" % (stmIdx+1))
				target = None
			if target:
				target = m.process([target])
			tempStatement = m.process(eqRight, tracker)
			result.append( MatchCase(target, unpacks, tempStatement) )
		return (left, execution.MakeMatchExec(node.loc, result), right)

# array (contents)
class ArrayMacro(SeqMacro):
	def __init__(s):
		super(ArrayMacro, s).__init__(progress = ProgressBase.Parser + 500)

	def symbol(s):
		return u"array"

	def construct(s, m, seq):
		return execution.MakeArrayExec(seq.loc, m.makeArray(seq))

# new (contents) or inherit (contents)
class ObjectMacro(OneSymbolMacro):
	def __init__(s, isInstance):
		super(ObjectMacro, s).__init__(progress = ProgressBase.Parser + 500)
		s.isInstance = isInstance

	def symbol(s):
		return u"new" if s.isInstance else u"inherit"

	def apply(s, m, left, node, right, _):
		if not right:
			return Error(node.loc, u"Emptiness after \"new\"")
		base = right.pop(0)
		base = m.process([base])
		if not right:
			seq = reader.ExpGroup(base.loc)
		else:
			seq = right.pop(0)
			if type(seq) != reader.ExpGroup:
				return Error(node.loc, u"Expected a (group) after \"new [base]\"")

		seq = m.makeSequence(seq.loc, seq.statements, False).execs if seq.nonempty() else []
		values = []
		assigns = []
		foundSet = False
		for assign in seq:
			if type(assign) == execution.SetExec:
				foundSet = True
				if assign.target:
					return Error(assign.loc, "Assignment inside object literal was not of form key=value")
				if assign.isLet:
					return Error(assign.loc, "\"let\" is redundant in an object literal")
				if assign.isExport:
					return Error(assign.loc, "\"export\" is redundant in an object literal")
				assign.isLet = True
				assigns.append(assign)
			elif type(assign) == execution.ImportAllExec:
				foundSet = True
				assigns.append(assign)
			else:
				if foundSet:
					return Error(assign.loc, "Found a stray value expression inside an object literal")
				else:
					values.append(assign)
		return (left, execution.MakeObjectExec(node.loc, base, values, assigns, s.isInstance), right)

# Final pass: Turn everything not swallowed by a macro into a value
class ValueMacro(Macro):
	def __init__(s):
		super(ValueMacro, s).__init__(progress = ProgressBase.Parser + 900)

	def match(s, left, node, right):
		c = type(node)
		return c == reader.QuoteExp or c == reader.NumberExp or c == reader.SymbolExp

	def apply(s, m, left, node, right, _):
		for case in switch(type(node)):
			if case(reader.QuoteExp):
				node = execution.StringLiteralExec(node.loc, node.content)
			elif case(reader.NumberExp):
				value = node.integer
				if node.dot:
					value += "."
				if node.decimal is not None:
					value += node.decimal
				node = execution.NumberLiteralExec(node.loc, float(value))
			elif case(reader.SymbolExp):
				if node.isAtom:
					node = execution.AtomLiteralExec(node.loc, node.content)
				else:
					node = execution.VarExec(node.loc, node.content)
			else:
				return Error(node.loc, "Internal error: AST node of indecipherable type %s found in a place that shouldn't be possible" % (type(node).__name__))

		return (left, node, right)

# "Nonstandard" macros

# (left) && (right)

class FancySplitterMacro(OneSymbolMacro):
	def __init__(s, progress):
		super(FancySplitterMacro, s).__init__(progress = progress)

	def apply(s, m, left, node, right, _):
		if not left:
			return Error(node.loc, u"Emptiness after \"%s\"" % (node.content))
		if not right:
			return Error(node.loc, u"Emptiness after \"%s\"" % (node.content))
		leftExe = m.process(left)
		rightExe = m.process(right)
		return s.expression(node.loc, leftExe, rightExe)

class AndMacro(FancySplitterMacro):
	def __init__(s):
		super(AndMacro, s).__init__(progress = ProgressBase.Parser + 605)

	def symbol(s):
		return "&&"

	def expression(s, loc, leftExe, rightExe):
		falseHere = execution.BooleanLiteralExec(loc, False)
		trueHere = execution.BooleanLiteralExec(loc, True)
		return ([],
			execution.IfExec(loc, False, leftExe,
				execution.IfExec(loc, False, rightExe, trueHere, falseHere), 
				falseHere),
			[])

class OrMacro(FancySplitterMacro):
	def __init__(s):
		super(OrMacro, s).__init__(progress = ProgressBase.Parser + 603)

	def symbol(s):
		return "||"

	def expression(s, loc, leftExe, rightExe):
		falseHere = execution.BooleanLiteralExec(loc, False)
		trueHere = execution.BooleanLiteralExec(loc, True)
		return ([],
			execution.IfExec(loc, False, leftExe, trueHere,
				execution.IfExec(loc, False, rightExe, trueHere, falseHere)),
			[])

# (left) || (right)

# User defined macro constructors
class UserMacro(OneSymbolMacro):
	def __init__(s, progress, symbol):
		super(UserMacro, s).__init__(progress = progress)
		s.symbolCache = symbol

	def symbol(s):
		return s.symbolCache

class SplitMacro(UserMacro):
	def __init__(s, progress, symbol):
		super(SplitMacro, s).__init__(progress, symbol)

	def apply(s, m, left, node, right, tracker):
		if not left: # Slight code redundancy with FancySplitter?
			return Error(node.loc, u"Emptiness after \"%s\"" % (node.content))
		if not right:
			return Error(node.loc, u"Emptiness after \"%s\"" % (node.content))
		return ([],
			execution.ApplyExec(node.loc,
				execution.ApplyExec(node.loc,
						execution.VarExec(node.loc, s.symbolCache),
						m.process(left)),
				m.process(right, tracker)),
			[])

class UnaryMacro(UserMacro):
	def __init__(s, progress, symbol):
		super(UnaryMacro, s).__init__(progress, symbol)

	def apply(s, m, left, node, right, tracker):
		if not right:
			return Error(node.loc, u"Emptiness after \"%s\"" % (s.symbol()))
		return (left,
				execution.ApplyExec(node.loc,
					execution.VarExec(node.loc, s.symbolCache),
					m.process(right, tracker)),
			[])

minimalMacros = [
	SetMacro(),
	ValueMacro()
]

defaultMacros = [
	MacroMacro(False), MacroMacro(True), ImportMacro(),
	DoMacro(), IfMacro(False), IfMacro(True), FunctionMacro(), MatchMacro(),
	ArrayMacro(), ObjectMacro(True), ObjectMacro(False),	
] + minimalMacros

shortCircuitBooleanMacros = [OrMacro(), AndMacro()]

def exeFromAst(ast):
	parser = Parser()
	result = parser.makeSequence(ast.loc, ast.statements) # TODO test to make sure it's a group
	if parser.errors:
		output = [errorFormat(e) for e in parser.errors]
		raise ParserException(u"\n".join(output))
	return result