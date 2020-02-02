# Execution tree classes

import sys
import os.path
from core import *
from util import unicodeJoin, quotedString, fileChars, utfOpen, streamable
import codecs
import reader, parser

# Values

class ExecutionExceptionFrame(object):
	def __init__(s, loc, what):
		s.loc = loc
		s.what = what

class ExecutionException(EmilyException):
	def __init__(s, loc, what, msg):
		super(ExecutionException, s).__init__(msg)
		s.msg = msg
		s.stack = []
		s.push(loc, what)
		for f in reversed(emilyDebugStack):
			s.stack.append(f)

	def push(s, loc, what):
		s.stack.append( ExecutionExceptionFrame(loc, what) )

class InternalExecutionException(Exception):
	pass

class LookupException(InternalExecutionException):
	def __init__(s, msg):
		super(LookupException, s).__init__(u"Lookup error: " + msg)

class TypeException(InternalExecutionException):
	def __init__(s, msg):
		super(TypeException, s).__init__(u"Type error: " + msg)

class LibraryException(InternalExecutionException):
	pass

def atomKeysOnly(fields=False):
	raise TypeException(u"Object has atom%s keys only" % (" or numeric" if fields else ""))

def intKeysOnly(fields=False):
	raise TypeException(u"Array has numeric keys only")

def noSuchKey(key, beingSet=False):
	raise LookupException(u"No such key: %s%s" % (key, " (trying to set)" if beingSet else ""))

scopeExportList = object()
macroExportList = object()

class EmilyValue(object):
	pass

# I hate this! It's not threadsafe, it's not... anything-safe. It's inefficient.
emilyDebugStack = []

class PythonFunctionValue(EmilyValue):
	def __init__(s, argCount, fn, startingArgs = []): # Takes ownership of startingArgs
		s.argCount = argCount
		s.fn = fn
		s.args = startingArgs

	def apply(s, value):
		if s.argCount == 0:
			return s.fn()
		newArgs = s.args + [value]
		if len(s.args)+1 >= s.argCount:
			return s.fn(*newArgs)
		return PythonFunctionValue(s.argCount, s.fn, newArgs)

# TODO: Can any code be shared with PythonFunctionValue?
class FunctionValue(EmilyValue):
	def __init__(s, argNames, exe, scope, startingArgs = []): # Takes ownership of startingArgs
		s.argNames = argNames
		s.exe = exe
		s.scope = scope
		s.args = startingArgs

	def apply(s, value):
		if not s.argNames:
			try:
				emilyDebugStack.append(ExecutionExceptionFrame(s.exe.loc, "Function call"))
				return s.exe.eval(s.scope)
			finally:
				emilyDebugStack.pop()
		newArgs = s.args + [value]
		if len(newArgs) >= len(s.argNames):
			scope = ObjectValue(s.scope)
			for idx in range(len(s.argNames)):
				scope.atoms[s.argNames[idx]] = newArgs[idx]
			try:
				emilyDebugStack.append(ExecutionExceptionFrame(s.exe.loc, "Function call"))
				return s.exe.eval(scope)
			finally:
				emilyDebugStack.pop()
		return FunctionValue(s.argNames, s.exe, s.scope, newArgs)

def isImpl(parent, child):
	if parent == child:
		return True
	if type(child) == bool:
		return parent == booleanPrototype
	if type(child) == float:
		return parent == numberPrototype
	if type(child) == unicode:
		return parent == stringPrototype
	if type(child) == ArrayValue:
		return parent == arrayPrototype
	if type(child) == ObjectValue:
		if parent == rootObject:
			return True
		while child.parent:
			child = child.parent
			if parent == child:
				return True
	return False

class MatchFunctionValue(EmilyValue):
	def __init__(s, matches, scope):
		s.matches = matches
		s.scope = scope

	def apply(s, value):
		for m in s.matches:
			if not m.targetExe or isImpl(m.targetExe.eval(s.scope), value):
				scope = s.scope
				if m.unpacks:
					scope = ObjectValue(scope)
					unpackIdx = 0
					for atom in m.unpacks:
						try:
							scope.atoms[atom.value] = value.apply(unpackIdx)
						except LookupException:
							raise InternalExecutionException("Match error: Unpack specification has too many values")
						unpackIdx += 1
				try:
					emilyDebugStack.append(ExecutionExceptionFrame(m.statement.loc, "Match"))
					return m.statement.eval(scope)
				finally:
					emilyDebugStack.pop() 
		raise InternalExecutionException("No match clause was met")

class SuperValue(EmilyValue):
	def __init__(s, parent, target):
		s.parent = parent
		s.target = target

	def apply(s, key):
		if type(key) != AtomLiteralExec:
			atomKeysOnly() # Raises
		return MethodPseudoValue.fetch(s.parent, key.value, s.target)

# Pseudovalue since it can never escape
class MethodPseudoValue(EmilyValue):
	def __init__(s, scope=None, owner=None, exe=None, pythonFunction=None):
		s.scope = scope
		s.owner = owner
		s.exe = exe
		s.pythonFunction = pythonFunction

	def call(s, target):
		if s.pythonFunction:
			return s.pythonFunction.apply(target)
		else:
			scope = ObjectValue(s.scope)
			scope.atoms['this'] = target
			scope.atoms['current'] = s.owner
			scope.atoms['super'] = SuperValue(s.owner.parent, target)
			try:
				emilyDebugStack.append(ExecutionExceptionFrame(s.exe.loc, "Method call"))
				return s.exe.eval(scope)
			finally:
				emilyDebugStack.pop()

	@staticmethod
	def fetch(source, key, this):
		value = source.innerLookup(key)
		if type(value) == MethodPseudoValue:
			return value.call(this)
		return value

class ObjectValue(EmilyValue):
	def __init__(s, parent=None, fields=None):
		s.atoms = {}
		s.parent = parent
		s.fields = fields

	def key(s, key):
		if type(key) == float:
			key = int(key)
		if type(key) == int:
			key = s.fields[key]
		if type(key) != AtomLiteralExec:
			atomKeysOnly(True) # Raises
		return key

	def innerLookup(s, key): # Already sanitized for atom correctness, method irrelevant
		if key in s.atoms:
			return s.atoms[key]
		if s.parent:
			return s.parent.innerLookup(key)
		noSuchKey(key) # Raises

	def lookup(s, key): # Already sanitized for atom correctness
		return MethodPseudoValue.fetch(s, key, s)

	def apply(s, key):
		try:
			key = s.key(key)
		except IndexError: # Used an excessive int key
			noSuchKey(key, False)
		return s.lookup(key.value)

	def innerAssign(s, isLet, key, value): # Again, sanitized for atom correctness
		if isLet or key in s.atoms:
			s.atoms[key] = value
		elif s.parent:
			s.parent.innerAssign(isLet, key, value)
		else:
			noSuchKey(key, True) # Raises

	def assign(s, isLet, key, value):
		try:
			key = s.key(key)
		except IndexError: # Used an excessive int key
			noSuchKey(key, True)
		return s.innerAssign(isLet, key.value, value)

globalPackageCache = {}

class PackageValue(EmilyValue):
	def __init__(s, tagbase, base):
		s.tagbase = tagbase
		s.base = base
		s.loaded = {}

	def apply(s, key):
		if type(key) != AtomLiteralExec or type(key.value) != unicode:
			atomKeysOnly() # Raises
		return s.lookup(key.value)

	def lookup(s, key): # Already sanitized for atom correctness
		if key in s.loaded:
			return s.loaded[key]
		isdir = False

		filename = os.path.join(s.base, key + ".em")
		filename = os.path.realpath(filename)
		if not os.path.isfile(filename):
			dirname = os.path.join(s.base, key)
			dirname = os.path.realpath(dirname)
			if os.path.isdir(dirname):
				isdir = True
				filename = dirname
			else:
				raise InternalExecutionException("Could not find file \"%s\" or directory \"%s\"" % (filename, dirname))

		if filename in globalPackageCache:
			value = globalPackageCache[filename]

			if value is None:
				raise InternalExecutionException("File \"%s\" attempted to recursively load itself while it was still executing" % filename)
		else:
			tag = s.tagbase + "." + key

			# FIXME: If it ever becomes possible to recover from errors, this will be a problem
			if isdir:
				value = PackageValue(tag, filename)
			else:
				globalPackageCache[filename] = None

				try:
					ast = reader.ast( fileChars(utfOpen(filename)), tag )
				except IOError:
					raise InternalExecutionException("Could not load file \"%s\"" % filename)

				ast = parser.exeFromAst(ast)
				value = ObjectValue()
				try:
					emilyDebugStack.append(ExecutionExceptionFrame(ast.loc, "Package load"))
					ast.eval(defaultScope, value)
				finally:
					emilyDebugStack.pop()

			globalPackageCache[filename] = value

		s.loaded[key] = value
		return value

# An attempt to work around a corner I wrote myself into: these special objects do nothing except
# consist of loaders for macros, which the system loads internally by calling lookup with a special key
# This is done with special objects so that calculating the macro object value can be deferred.
class LazyMacroLoader(EmilyValue):
	def __init__(s):
		s.cache = None

	def apply(s, key):
		if type(key) != AtomLiteralExec or key.value != macroExportList:
			raise InternalExecutionException(u'Internal error: Bad lookup on macro object')
		if not s.cache:
			s.cache = s.value()
		return s.cache

	def importObject(s):
		return None

class LazyMacroLambdaLoader(LazyMacroLoader):
	def __init__(s, fn):
		super(LazyMacroLambdaLoader, s).__init__()
		s.fn = fn

	def value(s):
		cache = s.fn()
		s.fn = None
		return cache

class PackageAliasValue(LazyMacroLoader):
	def __init__(s, path):
		super(PackageAliasValue, s).__init__()
		s.path = path
		s.objectCache = None

	def value(s):
		cache = libraryPackage
		for v in s.path:
			cache = cache.lookup(v)
		s.path = None
		s.objectCache = cache
		cache = cache.lookup(macroExportList)
		return cache

	def importObject(s):
		if not s.cache:
			s.cache = s.value()
		return s.objectCache

# Small chunk of standard library: The array prototype

arrayIteratorSource = object()
arrayIteratorIdx = object()

arrayIteratorPrototype = ObjectValue()
arrayIteratorPrototype.atoms['more'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, lambda x:toBoolean(x.atoms[arrayIteratorIdx] < len(x.atoms[arrayIteratorSource]))))
def arrayIteratorNextImpl(i):
	ary = i.atoms[arrayIteratorSource]
	idx = i.atoms[arrayIteratorIdx]
	try:
		x = ary[idx]
	except IndexError:
		raise InternalExecutionException(u'Called "next" on exhausted array iterator (index %d)' % idx)
	i.atoms[arrayIteratorIdx] += 1
	return x
arrayIteratorPrototype.atoms['next'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, arrayIteratorNextImpl))

arrayReverseIteratorPrototype = ObjectValue()
arrayReverseIteratorPrototype.atoms['more'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, lambda x:toBoolean(x.atoms[arrayIteratorIdx] > 0)))
def arrayReverseIteratorNextImpl(i):
	i.atoms[arrayIteratorIdx] -= 1
	x = i.atoms[arrayIteratorSource][ i.atoms[arrayIteratorIdx] ]
	return x
arrayReverseIteratorPrototype.atoms['next'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, arrayReverseIteratorNextImpl))

arrayPrototype = ObjectValue()
arrayPrototype.atoms['length'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, lambda x:float(len(x.values))))
arrayPrototype.atoms['append'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(2, lambda x,y:x.values.append(y)))
def arrayPopImpl(x):
	try:
		return x.values.pop()
	except IndexError:
		raise InternalExecutionException(u'Attempted pop on empty array')
arrayPrototype.atoms['pop'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, arrayPopImpl))

def arrayIteratorImpl(ary):
	x = ObjectValue(arrayIteratorPrototype)
	x.atoms[arrayIteratorSource] = ary.values # Assumes values array is never reassigned for an ArrayValue
	x.atoms[arrayIteratorIdx] = 0
	return x
arrayPrototype.atoms['iter'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, arrayIteratorImpl))

def arrayReverseIteratorImpl(ary):
	x = ObjectValue(arrayReverseIteratorPrototype)
	x.atoms[arrayIteratorSource] = ary.values # Assumes values array is never reassigned for an ArrayValue
	x.atoms[arrayIteratorIdx] = len(ary.values)
	return x
arrayPrototype.atoms['reverseIter'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, arrayReverseIteratorImpl))

class ArrayValue(EmilyValue):
	def __init__(s, values):
		s.values = values

	def apply(s, key):
		if type(key) == float:
			key = int(key) # IS ROUND-DOWN ACTUALLY GOOD?
		if type(key) == int:
			try:
				return s.values[key]
			except IndexError:
				raise InternalExecutionException(u'Index %d out of range' % key)
		if type(key) == AtomLiteralExec:
			return MethodPseudoValue.fetch(arrayPrototype, key.value, s)
		intKeysOnly()

	def assign(s, isLet, key, value):
		if type(key) == float:
			key = int(key)
		elif type(key) != int:
			intKeysOnly()
		s.values[key] = value
		return None

# Executable nodes

class Executable(Node):
	def __init__(s, loc): # , line, char
		super(Executable, s).__init__(loc)
		s.progress = ProgressBase.Executable

class InvalidExec(Executable):
	def __init__(s, loc):
		super(InvalidExec, s).__init__(loc)
		
	def __unicode__(s):
		return u"[Invalid node]"

	def eval(s, scope):
		raise ExecutionException(s.loc, "Invalid expression", "Tried to execute invalid program")

class SequenceExec(Executable):
	def __init__(s, loc, shouldReturn, hasScope, execs, macros = None):
		super(SequenceExec, s).__init__(loc)
		s.shouldReturn = shouldReturn
		s.hasScope = hasScope or bool(macros)
		s.execs = execs
		s.macros = macros

	def __unicode__(s):
		tags = (["Scoped"] if s.hasScope else []) + (["Returning"] if s.shouldReturn else [])
		return u"[Sequence%s %s]" % (("(%s)"%u", ".join(tags)) if tags else "", unicodeJoin(u" ", s.execs))

	def eval(s, scope, exportScope = None):
		exportList = None

		if s.hasScope:
			scope = ObjectValue(scope)
			if exportScope:
				exportList = set()
				scope.atoms[scopeExportList] = exportList

		for exe in s.execs:
			result = exe.eval(scope)

		if exportList is not None:
			for key in exportList:
				exportScope.atoms[key] = scope.atoms[key]
			if s.macros:
				exportScope.atoms[macroExportList] = s.macros

		return result if s.shouldReturn else None

class LiteralExec(Executable):
	def __init__(s, loc):
		super(LiteralExec, s).__init__(loc)

class StoredLiteralExec(Executable):
	def __init__(s, loc, value):
		super(StoredLiteralExec, s).__init__(loc)
		s.value = value

	def __unicode__(s):
		return u"[InternalLiteral]"

	def eval(s, scope):
		return s.value

class StringLiteralExec(StoredLiteralExec):
	def __unicode__(s):
		return u"[StringLiteral %s]" % (quotedString(s.value))

class BooleanLiteralExec(StoredLiteralExec):
	def __unicode__(s):
		return u"[BooleanLiteral %s]" % (boolToString(s.value))

class NumberLiteralExec(StoredLiteralExec):
	def __unicode__(s):
		return u"[NumberLiteral %s]" % (numberToString(s.value))

class AtomLiteralExec(LiteralExec, EmilyValue):
	def __init__(s, loc, value):
		super(AtomLiteralExec, s).__init__(loc)
		s.value = value

	def __unicode__(s):
		return u"[AtomLiteral %s]" % (s.value)

	def eval(s, scope):
		return s

	def apply(s, key): # Kludge: AtomLiteralExec, uniquely, also acts like a value
		if type(key) == AtomLiteralExec:
			return MethodPseudoValue.fetch(atomPrototype, key.value, s)
		atomKeysOnly()

class NullLiteralExec(LiteralExec):
	def __init__(s, loc):
		super(NullLiteralExec, s).__init__(loc)

	def __unicode__(s):
		return u"[NullLiteral]"

	def eval(s, scope):
		return None


class FalseLiteralExec(LiteralExec):
	def __init__(s, loc):
		super(NullLiteralExec, s).__init__(loc)

	def __unicode__(s):
		return u"[FalseLiteral]"

	def eval(s, scope):
		return None


class TrueLiteralExec(LiteralExec):
	def __init__(s, loc):
		super(NullLiteralExec, s).__init__(loc)

	def __unicode__(s):
		return u"[TrueLiteral]"

	def eval(s, scope):
		return None

class IfExec(Executable):
	def __init__(s, loc, loop, condClause, ifClause, elseClause):
		super(IfExec, s).__init__(loc)
		s.loop = loop
		s.condClause = condClause
		s.ifClause = ifClause
		s.elseClause = elseClause

	def __unicode__(s):
		return u"[%s %s]" % ("While" if s.loop else "If",
			unicodeJoin(u" ", [s.condClause, s.ifClause] + ([s.elseClause] if s.elseClause else [])))

	def eval(s, scope):
		if not s.loop:
			cond = s.condClause.eval(scope)
			if cond:
				if s.ifClause:
					cond = None
					return s.ifClause.eval(scope)
				else:
					return cond
			if s.elseClause:
				return s.elseClause.eval(scope)
		else:
			while s.condClause.eval(scope):
				s.ifClause.eval(scope)
		return None

class MakeMatchExec(Executable):
	def __init__(s, loc, matches):
		super(MakeMatchExec, s).__init__(loc)
		s.matches = matches

	def __unicode__(s):
		result = u"[Match"
		for match in s.matches:
			result += " [Case %s [%s] %s]" % (unicode(match.targetExe), unicodeJoin(", ", match.unpacks), unicode(match.statement))
		result += "]"
		return result

	def eval(s, scope):
		return MatchFunctionValue(s.matches, scope)

class VarExec(Executable):
	def __init__(s, loc, symbol):
		super(VarExec, s).__init__(loc)
		s.symbol = symbol

	def __unicode__(s):
		return u"[Var %s]" % (s.symbol)

	def eval(s, scope):
		try:
			return scope.lookup(s.symbol)
		except InternalExecutionException as e:
			raise ExecutionException(s.loc, u"Variable read", unicode(e))
		except KeyboardInterrupt:
			raise ExecutionException(s.loc, u"Variable read", u"Ctrl-C")

class SetExec(Executable):
	def __init__(s, loc, isLet, isMethod, isField, isExport, target, index, valueClause): # TODO: indexClauses
		super(SetExec, s).__init__(loc)
		s.isLet = isLet
		s.isMethod = isMethod
		s.isField = isField
		s.isExport = isExport
		s.target = target
		s.index = index
		s.valueClause = valueClause

	def __unicode__(s):
		return u"[%s %s %s %s]" % (u"Export" if s.isExport else (u"Let" if s.isLet else u"Set"), unicode(s.target) if s.target else u"Scope", unicode(s.index), unicode(s.valueClause))

	def eval(s, scope, targetOverride = None, indexOverride = None):
		if targetOverride:
			target = targetOverride
		elif s.target:
			target = s.target.eval(scope)
		else:
			target = scope

		if indexOverride: # FIXME: Will act surprisingly if index evaluates to None
			index = indexOverride
		else:
			index = s.index.eval(scope)

		if s.isMethod:
			value = MethodPseudoValue(scope, target, s.valueClause)
		else:
			value = s.valueClause.eval(scope)

		if s.isExport:
			if scopeExportList not in scope.atoms:
				raise ExecutionException(s.loc, u"Assignment", u"\"export\" in unexpected place")
			scope.atoms[scopeExportList].add(index.value)

		try:
			target.assign(s.isLet or s.isExport, index, value)
		except InternalExecutionException as e:
			raise ExecutionException(s.loc, u"Assignment", unicode(e))
		except KeyboardInterrupt:
			raise ExecutionException(s.loc, u"Assignment", u"Ctrl-C")

		return None

class ImportAllExec(Executable):
	def __init__(s, loc, sourceClause, isExport = False): # TODO: indexClauses
		super(ImportAllExec, s).__init__(loc)
		s.sourceClause = sourceClause
		s.isExport = isExport

	def __unicode__(s):
		return u"[ImportAll %s]" % (unicode(s.sourceClause))

	def eval(s, scope, targetOverride = None, indexOverride = None):
		scopeExport = None
		if s.isExport:
			if scopeExportList not in scope.atoms:
				raise ExecutionException(s.loc, u"Assignment", u"\"export\" in unexpected place")
			scopeExport = scope.atoms[scopeExportList]

		source = s.sourceClause.eval(scope)
		if not isinstance(source, ObjectValue):
			raise ExecutionException(s.loc, u"Import", u"Attempted to import * from something other than an object")

		if targetOverride:
			target = targetOverride
		else:
			target = scope

		try:
			for key in source.atoms:
				if key != macroExportList:
					value = source.lookup(key)
					target.innerAssign(True, key, value)
					if scopeExport is not None:
						scopeExport.add(key)
		except InternalExecutionException as e:
			raise ExecutionException(s.loc, u"Import", unicode(e))
		except KeyboardInterrupt:
			raise ExecutionException(s.loc, u"Import", u"Ctrl-C")

		return None

class ApplyExec(Executable):
	def __init__(s, loc, f, arg): # f for function
		super(ApplyExec, s).__init__(loc) # FIXME: f.location
		s.f = f
		s.arg = arg

	def __unicode__(s):
		return u"[Apply %s]" % (unicodeJoin(u" ", [s.f, s.arg]))

	def eval(s, scope): # This is the core "apply X to Y" of the entire engine
		try:
			value = s.f.eval(scope)
			arg = s.arg.eval(scope)

			# Normal case
			if isinstance(value, EmilyValue):
				return value.apply(arg)
			
			# Screen for "weird" values
			wasString = False
			if type(value) == unicode: # String not only has a prototype, it's indexable
				if type(arg) == float:
					arg = int(arg)
				if type(arg) == int:
					try:
						return value[arg]
					except IndexError:
						raise InternalExecutionException(u'Index %d out of range' % arg)

				prototype = stringPrototype
				wasString = True
			elif type(value) == float or type(value) == int:
				prototype = numberPrototype
			elif type(value) == bool:
				prototype = booleanPrototype
			elif value is None:
				prototype = nullPrototype
			else:
				raise InternalExecutionException(u"Don't know how to apply value of Python-type %s: %s\n\tThis error probably indicates a bug in the interpreter." % (type(value), unicode(value)))

			if type(arg) != AtomLiteralExec:
				atomKeysOnly(wasString)
			return MethodPseudoValue.fetch(prototype, arg.value, value)			
		except InternalExecutionException as e:
			raise ExecutionException(s.loc, u"Application", unicode(e))
		except KeyboardInterrupt:
			raise ExecutionException(s.loc, u"Application", u"Ctrl-C")

class MakeFuncExec(Executable):
	def __init__(s, loc, args, body): # f for function
		super(MakeFuncExec, s).__init__(loc) # FIXME: f.location
		s.args = args
		s.body = body

	def __unicode__(s):
		return u"[Function [%s] %s]" % (u", ".join(s.args), unicode(s.body))

	def eval(s, scope):
		return FunctionValue(s.args, s.body, scope)

class MakeArrayExec(Executable):
	def __init__(s, loc, contents):
		super(MakeArrayExec, s).__init__(loc)
		s.contents = contents

	def __unicode__(s):
		return u"[Array %s]" % (unicodeJoin(u", ", s.contents))

	def eval(s, scope):
		values = [None] * len(s.contents)
		idx = 0
		for exe in s.contents:
			values[idx] = exe.eval(scope)
			idx += 1
		return ArrayValue(values)

rootObject = ObjectValue() # Singleton "root object"

class MakeObjectExec(Executable):
	def __init__(s, loc, base, values, assigns, isInstance):
		super(MakeObjectExec, s).__init__(loc)
		s.base = base
		s.values = values
		s.assigns = assigns
		s.isInstance = isInstance

	def __unicode__(s):
		return u"[%s %s [%s]]" % ("New" if s.isInstance else "Inherit", unicode(s.base), unicodeJoin(u", ", s.values))

	def eval(s, scope):
		base = s.base.eval(scope)
		if (base and type(base) != ObjectValue):
			raise ExecutionException(s.loc, u"Object construction", u"Cannot inherit from non-object")
		if base == rootObject: # Tiny optimization: Don't actually inherit from Object
			base = None
		result = ObjectValue(base)
		innerScope = ObjectValue(scope)
		innerScope.atoms['current'] = result

		infields = base.fields if base else None
		if s.isInstance and infields: # FIXME: This calls method fields even when not needed
			for field in infields:
				result.assign(True, field, base.apply(field))
		valueProgress = 0
		if (len(s.values) if s.values else 0) > (len(infields) if infields else 0):
			raise ExecutionException(s.loc, u"Object construction", u"Tried to specify more values in \"new\" than this object has fields")
		for exe in s.values:
			value = exe.eval(innerScope)
			result.atoms[ infields[valueProgress].value ] = value
			valueProgress += 1
		for exe in s.assigns:
			if isinstance(exe, SetExec):
				key = exe.index.eval(innerScope) # do this early for field handling
				if exe.isField:
					if type(key) != AtomLiteralExec:
						raise ExecutionException(exe.loc, u"Object construction", "Objects have atom keys only")
					if not result.fields:
						result.fields = list(infields) if infields else []
					result.fields.append(key)
			else:
				key = None
			exe.eval(innerScope, result, key)
		if not result.fields:
			result.fields = infields
		return result

# Base scope ("standard library")

# Basics/math

defaultScope = ObjectValue()
defaultScope.atoms['null'] = None
defaultScope.atoms['nullfn'] = PythonFunctionValue(1, lambda x: None)
defaultScope.atoms['false'] = False # FIXME
defaultScope.atoms['true'] = True     # FIXME
defaultScope.atoms['Object'] = rootObject
defaultScope.atoms['Array'] = arrayPrototype
defaultScope.atoms['with'] = PythonFunctionValue(2, lambda x,y: y.apply(x))
def makeBinop(f):
	def wrapper(x, y):
		try:
			return f(x, y)
		except TypeError:
			raise InternalExecutionException(u"Tried to do arithmetic with bad types: %s vs %s" % (type(x), type(y)))
	return PythonFunctionValue(2, wrapper)
defaultScope.atoms['+'] = makeBinop(lambda x,y: x + y)
defaultScope.atoms['-'] = makeBinop(lambda x,y: x - y)
defaultScope.atoms['*'] = makeBinop(lambda x,y: x * y)
defaultScope.atoms['/'] = makeBinop(lambda x,y: x / y)
defaultScope.atoms['%'] = makeBinop(lambda x,y: x % y)

# Boolean math

def toBoolean(x):
	return True if x else False
defaultScope.atoms['boolean'] = PythonFunctionValue(1, toBoolean)
defaultScope.atoms['not'] = PythonFunctionValue(1, lambda x: toBoolean(not x))
defaultScope.atoms['neg'] = PythonFunctionValue(1, lambda x: -x)
defaultScope.atoms['and'] = PythonFunctionValue(2, lambda x,y: toBoolean(x and y))
defaultScope.atoms['or'] = PythonFunctionValue(2, lambda x,y: toBoolean(x or y))
defaultScope.atoms['xor'] = PythonFunctionValue(2, lambda x,y: toBoolean(bool(x) != bool(y)))
defaultScope.atoms['=='] = PythonFunctionValue(2, lambda x,y: toBoolean(x == y if (type(x) is not bool and type(y) is not bool) else x is y))
defaultScope.atoms['!='] = PythonFunctionValue(2, lambda x,y: toBoolean(x != y if (type(x) is not bool and type(y) is not bool) else x is not y))
defaultScope.atoms['<']  = PythonFunctionValue(2, lambda x,y: toBoolean(x <  y))
defaultScope.atoms['<='] = PythonFunctionValue(2, lambda x,y: toBoolean(x <= y))
defaultScope.atoms['>']  = PythonFunctionValue(2, lambda x,y: toBoolean(x >  y))
defaultScope.atoms['>='] = PythonFunctionValue(2, lambda x,y: toBoolean(x >= y))
defaultScope.atoms['is'] = PythonFunctionValue(2, lambda x,y: toBoolean(isImpl(x,y)))

# Dubious, intentionally "undocumented"
def debugPrint(obj):
	print u"----\nDEBUG: %s" % (type(obj))
	print u"Id: %s" % (id(obj))
	if type(obj) == unicode or type(obj) == float:
		print u"Value: %s" % obj
	parent = getattr(obj, 'parent', None)
	if parent:
		print u"Parent id: %s" % (id(parent))
	atoms = getattr(obj, 'atoms', None)
	if atoms:
		print u"Atoms: %s" % atoms
	values = getattr(obj, 'values', None)
	if values:
		print u"Values: %s" % values
	print u"----"
defaultScope.atoms['DEBUG'] = PythonFunctionValue(1, debugPrint)

# Macro support

def makeMacroChecks(progress, symbol):
	if progress < 0 or progress >= 1000:
		raise Exception("Macro progress must be between 0 and 999 inclusive")
	if type(symbol) != unicode:
		raise Exception("Macro symbol is not a symbol")
	
def makeSplitMacro(progress, symbol):
	makeMacroChecks(progress, symbol)
	return parser.SplitMacro(ProgressBase.Parser + progress, symbol)
defaultScope.atoms['splitMacro'] = PythonFunctionValue(2, makeSplitMacro)

def makeUnaryMacro(progress, symbol):
	makeMacroChecks(progress, symbol)
	return parser.UnaryMacro(ProgressBase.Parser + progress, symbol)
defaultScope.atoms['unaryMacro'] = PythonFunctionValue(2, makeUnaryMacro)


# Dict

dictObjectData = object()
dictObjectPrototype = ObjectValue()

def dictGet(obj, key):
	d = obj.atoms.get(dictObjectData)
	if d and key in d:
		return d[key]
	else:
		raise InternalExecutionException("No such key")
dictObjectPrototype.atoms['get'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(2, dictGet))

def dictSet(obj, key, value):
	d = obj.atoms.get(dictObjectData)
	if not d:
		d = dict()
		obj.atoms[dictObjectData] = d
	d[key] = value
dictObjectPrototype.atoms['set'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(3, dictSet))

def dictHas(obj, key):
	d = obj.atoms.get(dictObjectData)
	return toBoolean(d and key in d)
dictObjectPrototype.atoms['has'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(2, dictHas))

def dictDel(obj, key):
	d = obj.atoms.get(dictObjectData)
	if d and key in d:
		del d[key]
	else:
		raise InternalExecutionException("No such key")
dictObjectPrototype.atoms['del'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(2, dictDel))

def dictIter(obj):
	d = obj.atoms.get(dictObjectData)
	# NOTES:
	# - This is less efficient than it should be. There's no good reason to allocate a whole array here.
	# - This is sorted to make certain tests run more predictably. This should not be considered a spec behavior.
	# - If I'm going to overload stringIteratorImpl like this, shouldn't I rename it?
	return stringIteratorImpl(sorted(d.keys()) if d else [])
dictObjectPrototype.atoms['iter'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, dictIter))

defaultScope.atoms['Dict'] = dictObjectPrototype

# IO : Output

fileObjectHandle = object()
fileObjectNextIn = object()
fileObjectLastNewline = object()
infileObjectPrototype = ObjectValue()
outfileObjectPrototype = ObjectValue()

def printable(x):
	if x is None:
		v = u"null"
	elif type(x) == bool:
		v = u"true" if x else u"false"
	elif type(x) == float and int(x) == x:
		v = unicode(int(x))
	else:
		v = unicode(x)
	return streamable( v )

def setLooper(into, key, fn):
	looperValue = None
	def looper(x):
		fn(into, x)
		return looperValue # Have to do it like this in case it was overridden
	looperValue = PythonFunctionValue(1, looper)
	into.atoms[key] = looperValue

def writeWrapper(obj, x):
	handle = obj.atoms[fileObjectHandle]
	obj.atoms[fileObjectLastNewline] = True
	handle.write( printable(x) ) # FIXME: Unicode

def flushWrapper(obj):
	handle = obj.atoms[fileObjectHandle]
	handle.flush()
flushWrapperValue = MethodPseudoValue(pythonFunction=PythonFunctionValue(1,flushWrapper))

def closeWrapper(obj):
	handle = obj.atoms[fileObjectHandle]
	handle.close()
closeWrapperValue = MethodPseudoValue(pythonFunction=PythonFunctionValue(1,closeWrapper))

def printWrapper(obj, x):
	handle = obj.atoms[fileObjectHandle]
	if type(x) == unicode and x.endswith('\n'):
		obj.atoms[fileObjectLastNewline] = True
		handle.flush()
	else:
		if obj.atoms[fileObjectLastNewline]:
			obj.atoms[fileObjectLastNewline] = False
		else:
			handle.write(' ')
	handle.write( printable(x) ) # FIXME: Unicode

def printlnWrapper(obj, x):
	handle = obj.atoms[fileObjectHandle]
	handle.write( printable(x) ) # FIXME: Unicode
	handle.write( "\n" ) # FIXME: Unicode
	obj.atoms[fileObjectLastNewline] = True

def makeOutfileObject(handle):
	obj = ObjectValue(outfileObjectPrototype)
	obj.atoms[fileObjectHandle] = handle
	obj.atoms[fileObjectLastNewline] = True
	setLooper(obj, 'write', writeWrapper)
	obj.atoms['flush'] = flushWrapperValue
	setLooper(obj, 'print', printWrapper)
	setLooper(obj, 'println', printlnWrapper)
	obj.atoms['close'] = closeWrapperValue
	return obj

stdoutObject = makeOutfileObject(sys.stdout)
defaultScope.atoms['stdout'] = stdoutObject
stderrObject = makeOutfileObject(sys.stderr)
defaultScope.atoms['stderr'] = stderrObject
defaultScope.atoms['print'] = stdoutObject.atoms['print']
defaultScope.atoms['println'] = stdoutObject.atoms['println']

defaultScope.atoms['exit'] = PythonFunctionValue(1, lambda x: sys.exit(int(x)))
defaultScope.atoms['ln'] = u"\n"
defaultScope.atoms['argv'] = None # Note: This value will actually be seen by imported files.

def failImpl(s):
	result = u"Program signaled failure"
	if (s is not None):
		result += u": "
		result += s
	raise InternalExecutionException(result)
defaultScope.atoms['fail'] = PythonFunctionValue(1, failImpl)

fileObject = ObjectValue()
defaultScope.atoms['file'] = fileObject
def makeOutOpen(arg):
	def outOpen(filename):
		try:
			return makeOutfileObject(open(filename, arg))
		except IOError:
			raise InternalExecutionException("Could not open file \"%s\" for writing" % filename)
	return PythonFunctionValue(1, outOpen)
fileObject.atoms['out'] = makeOutOpen("w")
fileObject.atoms['append'] = makeOutOpen("a")

# IO: Input

def fileNextCached(obj): # If None: not known. If '': at eof. 
	nextIn = obj.atoms[fileObjectNextIn]
	if nextIn is None:
		handle = obj.atoms[fileObjectHandle]
		nextIn = handle.read(1)
		obj.atoms[fileObjectNextIn] = nextIn
	return nextIn

def fileMore(obj):
	value = fileNextCached(obj)
	return value != ''
fileMoreValue = MethodPseudoValue(pythonFunction=PythonFunctionValue(1,fileMore))

def filePeek(obj):
	value = fileNextCached(obj)
	if value == '':
		raise LibraryException("Read on filehandle with no data")
	return value
filePeekValue = MethodPseudoValue(pythonFunction=PythonFunctionValue(1,filePeek))

def fileNext(obj):
	value = filePeek(obj)
	obj.atoms[fileObjectNextIn] = None
	return value
fileNextValue = MethodPseudoValue(pythonFunction=PythonFunctionValue(1,fileNext))

def makeInfileObject(handle):
	obj = ObjectValue(infileObjectPrototype)
	obj.atoms[fileObjectHandle] = handle
	obj.atoms[fileObjectNextIn] = None
	obj.atoms['more'] = fileMoreValue
	obj.atoms['next'] = fileNextValue
	obj.atoms['peek'] = filePeekValue
	obj.atoms['close'] = closeWrapperValue
	return obj

# FIXME: What about binary mode?
def makeInOpen(filename):
	try:
		return makeInfileObject(utfOpen(filename))
	except IOError:
		raise InternalExecutionException("Could not open file \"%s\" for reading" % filename)
fileObject.atoms['in'] = PythonFunctionValue(1,makeInOpen)

stdinObject = makeInfileObject( codecs.getreader('utf-8')(sys.stdin) )
defaultScope.atoms['stdin'] = stdinObject

# IO: Filesystem

def toBooleanWrap(fn):
	def wrap(x):
		return toBoolean(fn(x))
	return wrap

pathObject = ObjectValue()
fileObject.atoms['path'] = pathObject

pathObject.atoms['join'] = PythonFunctionValue(2, os.path.join)
pathObject.atoms['normalize'] = PythonFunctionValue(1, os.path.realpath)
pathObject.atoms['isFile'] = PythonFunctionValue(1, toBooleanWrap(os.path.isfile))
pathObject.atoms['isDir'] = PythonFunctionValue(1, toBooleanWrap(os.path.isdir))
pathObject.atoms['dir'] = PythonFunctionValue(1, os.path.dirname)
pathObject.atoms['file'] = PythonFunctionValue(1, os.path.basename)
pathObject.atoms['entryFile'] = None

# String garbage

charObject = ObjectValue()
defaultScope.atoms['char'] = charObject
charObject.atoms['isNonLineSpace'] = PythonFunctionValue(1, toBooleanWrap(reader.isNonLineSpace))
charObject.atoms['isLineSpace'] = PythonFunctionValue(1, toBooleanWrap(reader.isLineSpace))
charObject.atoms['isSpace'] = PythonFunctionValue(1, toBooleanWrap(lambda x: reader.isLineSpace(x) or reader.isNonLineSpace(x)))
charObject.atoms['isQuote'] = PythonFunctionValue(1, toBooleanWrap(reader.isQuote))
charObject.atoms['isOpenParen'] = PythonFunctionValue(1, toBooleanWrap(reader.isOpenParen))
charObject.atoms['isCloseParen'] = PythonFunctionValue(1, toBooleanWrap(reader.isCloseParen))
charObject.atoms['isParen'] = PythonFunctionValue(1, toBooleanWrap(lambda x: reader.isOpenParen(x) or reader.isCloseParen(x)))
charObject.atoms['isDigit'] = PythonFunctionValue(1, toBooleanWrap(reader.isDigit))
charObject.atoms['isIdStart'] = PythonFunctionValue(1, toBooleanWrap(reader.isIdStart))
charObject.atoms['isIdContinue'] = PythonFunctionValue(1, toBooleanWrap(reader.isIdContinue))

# Booleans

booleanPrototype = ObjectValue()
def booleanToNumber(x):
	return 1 if x else 0
def booleanToString(x):
	return u'true' if x else u'false'
booleanPrototype.atoms['toString'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, booleanToString))
booleanPrototype.atoms['toNumber'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, booleanToNumber))

defaultScope.atoms['Boolean'] = booleanPrototype

# Numbers

numberPrototype = ObjectValue()
def numberToString(x):
	return unicode(int(x) if x == int(x) else x)
numberPrototype.atoms['toString'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, numberToString))
numberPrototype.atoms['toNumber'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, lambda x:x))

defaultScope.atoms['Number'] = numberPrototype

# Strings

stringPrototype = ObjectValue()

# Reuses arrayIteratorPrototype
stringPrototype.atoms['length'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, lambda x:float(len(x))))
def stringIteratorImpl(ary):
	x = ObjectValue(arrayIteratorPrototype)
	x.atoms[arrayIteratorSource] = ary # Store "hidden" values in object
	x.atoms[arrayIteratorIdx] = 0
	return x
stringPrototype.atoms['iter'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, stringIteratorImpl))
def stringReverseIteratorImpl(ary):
	x = ObjectValue(arrayReverseIteratorPrototype)
	x.atoms[arrayIteratorSource] = ary # Store "hidden" values in object
	x.atoms[arrayIteratorIdx] = len(ary)
	return x
stringPrototype.atoms['reverseIter'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, stringReverseIteratorImpl))
stringPrototype.atoms['toString'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, lambda x:x))
def toNumberImpl(x):
	try:
		return float(x)
	except ValueError:
		raise LibraryException("Could not convert to number: %s" % x)
stringPrototype.atoms['toNumber'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, toNumberImpl))

defaultScope.atoms['String'] = stringPrototype

# Atoms

atomPrototype = ObjectValue()
atomPrototype.atoms['toString'] = MethodPseudoValue(pythonFunction=PythonFunctionValue(1, lambda x:x.value))

# FIXME: Null has a prototype?

nullPrototype = ObjectValue()
nullPrototype.atoms['toString'] = "null"

# Packages

profileScope = ObjectValue()

def setEntryFile(filename):
	project = PackageValue("project", os.path.dirname(os.path.realpath(filename)))
	pathObject.atoms['entryFile'] = unicode(filename)
	defaultScope.atoms['project'] = project
	profileScope.atoms['project'] = project

# FIXME: This assumes emily is being executed out of its own VCS repo
libraryPackage = PackageValue("package", os.path.realpath(os.path.join(os.path.join(os.path.dirname(__file__), ".."), "library")))

defaultScope.atoms['package'] = libraryPackage
profileScope.atoms['package'] = libraryPackage
defaultScope.atoms['project'] = ObjectValue()
profileScope.atoms['project'] = defaultScope.atoms['project']

profileScope.atoms['minimal'] = LazyMacroLambdaLoader(lambda: parser.minimalMacros)
profileScope.atoms['default'] = LazyMacroLambdaLoader(lambda: parser.defaultMacros)
profileScope.atoms['shortCircuitBoolean'] = LazyMacroLambdaLoader(lambda: parser.shortCircuitBooleanMacros)
profileScope.atoms['experimental'] = PackageAliasValue(["emily", "profile", "experimental"])

# TODO: directory

# Used to test exporting

def debugScopeDump(obj):
	for key in sorted(obj.atoms.keys()):
		value = obj.atoms[key]
		if key == macroExportList:
			print "[Macros: %s]" % (len(value))
		else:
			if type(value) == MethodPseudoValue:
				value = "[Method]"
			print printable(key) + ": " + printable(value)
