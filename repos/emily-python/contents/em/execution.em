# Execution tree classes, code for values and default scope

profile experimental

from project.util import *
from project.core import *
from project.type import
	TypedNode, ReferType, UnitType, BooleanType, NumberType, StringType, AtomType, FunctionType, UnknowableType, Val

# Execution tree

export Executable = inherit TypedNode
	progress = ProgressBase.executable

	method fail = function (msg)
		fail
			nullJoin array
				"Execution failed at "
				this.loc
				":\n\t"
				msg

export InvalidExec = inherit Executable
	toString = "[Invalid node]"

	eval = function (scope)
		this.fail "Tried to execute invalid program"

	check = function (scope)
		this.fail "Tried to typecheck invalid program"

export SequenceExec = inherit Executable
	field shouldReturn = false
	field hasScope = false
	field method execs = array()
	field macros = null
	field type = null
	field typeScope = null # Memo-- used only by debug.em

	method evalSequence = function (scope, exportScope)
		let exportList = null
		if (this.hasScope || this.macros)
			scope = new ObjectValue(scope)
			if exportScope
				exportList = array()
				scope.atoms.set scopeExportList exportList
		let i = this.execs.iter
		let result = null
		while (i.more)
			result = i.next.eval(scope)
		if (exportList != null)
			let i = exportList.iter
			while (i.more)
				let key = i.next
				exportScope.atoms.set key (scope.atoms.get key)
			if (this.macros)
				exportScope.atoms.set macroExportList (this.macros)
		if (this.shouldReturn)
			result
		else
			NullValue

	method eval = function (scope)
		this.evalSequence scope null

	method check = function (scope)
		if (this.hasScope)
			let innerScope = new ChainedDict
			innerScope.set chainParent scope
			scope = innerScope
			this.typeScope = scope

		let i = this.execs.iter
		while (i.more)
			let exe = i.next
			if (is SetExec exe && exe.isLet)
				scope.set (exe.indexClause.value) new Val

		i = this.execs.iter
		let last = null

		while (i.more)
			last = i.next
			last.check (scope)

		if (last)
			this.unify last
		else
			this.type = UnitType

	method toString = do
		let tags = array()
		if (this.hasScope)
			tags.append "Scoped"
		if (this.shouldReturn)
			tags.append "Returning"
		nullJoin array
			"[Sequence"
			if (tags.length)
				nullJoin array("(", join ", " tags, ")")
			else
				""
			" "
			join " " (this.execs)
			"]"

export UserMacroList = inherit Executable
	field contents = null
	field isExport = false
	field isProfile = false
	field payload = null

	toString = "[Misplaced macro node]"

export LiteralExec = inherit Executable
	field value = null

	check = nullfn

export StoredLiteralExec = inherit LiteralExec
	toString = "[InternalLiteral]"
	type = null # TODO: Should be some special "Unresolvable" type, shared with InvalidExec

	method eval = function (scope)
		this.value

export StringLiteralExec = inherit LiteralExec
	method toString = nullJoin array("[StringLiteral ", quotedString(this.value), "]")
	type = StringType

	method eval = function (scope)
		new StringValue(this.value)

export BooleanLiteralExec = inherit LiteralExec
	method toString = nullJoin array("[BooleanLiteral ", this.value, "]")
	type = BooleanType

	method eval = function (scope)
		if (this.value) (TrueValue) else (FalseValue)

export NumberLiteralExec = inherit LiteralExec
	method toString = nullJoin array("[NumberLiteral ", this.value, "]")
	type = NumberType

	method eval = function (scope)
		new NumberValue(this.value)

export AtomLiteralExec = inherit LiteralExec
	method toString = nullJoin array("[AtomLiteral ", this.value, "]")
	type = AtomType # TODO: Atom types should be special

	method eval = function (scope)
		this

	# For questionable reasons, AtomLiteralExec (and no other Executable) doubles as its own value
	method apply = makePrototypeApply(atomValuePrototype, this)

# Does not inherit LiteralExec because it holds no value
export NullLiteralExec = inherit Executable
	toString = "[NullLiteral]"
	type = UnitType # TODO: No. No

	method eval = function (scope)
		NullValue

	check = nullfn

export VarExec = inherit Executable
	field symbol = null

	method toString = nullJoin array("[Var ", this.symbol, "]")

	method eval = function (scope)
		scope.lookup (this.symbol)

	field type = null
	method check = function (scope)
		let var = scope.get (this.symbol)
		if (var == chainNotFound)
			fail
				"Variable " + this.symbol + " not found during typecheck at " + this.loc.toString
		this.unify (var)

export ApplyExec = inherit Executable
	field fn = null
	field arg = null
	field type = null

	method toString = nullJoin array
		"[Apply "
		this.fn
		" "
		this.arg
		"]"

	method eval = function (scope)
		this.fn.eval(scope).apply (this.arg.eval(scope))

	method check = function (scope)
		this.fn.check scope
		this.arg.check scope
		this.fn.unifyArgResult(this.arg, this)

export SetExec = inherit Executable
	field isLet = false
	field isMethod = false
	field isField = false
	field isExport = false
	field targetClause = null
	field indexClause = null
	field valueClause = null
	type = UnitType

	method toString = nullJoin array
		"["
		if (this.isExport) ("Export") elif (this.isLet) ("Let") else ("Set")
		" "
		if (this.targetClause) (this.targetClause) else ("Scope")
		" "
		this.indexClause
		" "
		this.valueClause
		"]"

	method setEval = function (scope, target, index)
		let value = if (this.isMethod)
			new FunctionMethodPseudoValue(scope, target, this.valueClause)
		else
			this.valueClause.eval scope

		target.assign(or (this.isLet) (this.isExport), index, value)

	method eval = function (scope)
		let target = do
			if (this.targetClause)
				this.targetClause.eval scope
			else
				scope
		let index = this.indexClause.eval scope
		
		this.setEval(scope, target, index)			
		
		if (this.isExport)
			if (!scope.atoms.has scopeExportList)
				this.fail "\"export\" in unexpected place"
			else
				let exportList = scope.atoms.get scopeExportList
				exportList.append (index.value) # Just assume it's an atom

		NullValue

	method check = function (scope)
		if (this.targetClause) (this.targetClause.check scope)
		if (this.indexClause)  (this.indexClause.check scope)
		if (this.valueClause)  (this.valueClause.check scope)
		if (!this.targetClause) # Check type of indexClause? TODO: Indexing
			scope.get(this.indexClause.value).unify(this.valueClause)

export ImportAllExec = inherit Executable
	field sourceClause = null
	field isExport = false
	type = UnitType

	method toString = nullJoin array
		"[ImportAll "
		this.sourceClause
		"]"

	method setEval = function (scope, targetOverride, _)
		let source = this.sourceClause.eval(scope)
		let exportList = null

		if (this.isExport)
			if (!scope.atoms.has scopeExportList)
				this.fail "\"export\" in unexpected place"
			else
				exportList = scope.atoms.get scopeExportList

		if (!is ObjectValue source)
			this.fail "Attempted to import * from something other than an object"

		let target = null
		if (targetOverride)
			target = targetOverride
		else
			target = scope

		let i = source.atoms.iter
		while (i.more)
			let key = i.next
			let value = source.lookup(key)
			if (key != macroExportList)
				target.atoms.set key value # FIXME: Should this be done via a method on target?
				if (exportList != null)
					exportList.append(key)

		NullValue

	method eval = function (scope)
		this.setEval(scope, null, null)

	method check = function (scope)
		if (this.sourceClause) (this.sourceClause.check scope)

export MakeFuncExec = inherit Executable
	field args = null
	field body = null
	field type = null
	field typeScope = null # Memo-- used by debug.em and compiler

	method toString = nullJoin array
		"[Function ["
		join ", " (this.args)
		"] "
		this.body
		"]"

	method eval = function(scope)
		new FunctionValue(this.args, this.body, scope)

	method check = function(scope)
		let loc = this.loc
		let fnResult = new Val(loc)
		let fnType = new FunctionType
			new Val(loc), fnResult
		let i = this.args.reverseIter
		if (i.more)
			let alreadyArgs = false

			let innerScope = new ChainedDict
			innerScope.set chainParent scope
			scope = innerScope
			this.typeScope = scope

			while (i.more)
				let arg = i.next
				if (alreadyArgs)
					fnType = new FunctionType
						new Val(loc), new Val(loc, fnType)
				else
					alreadyArgs = true
				scope.set arg (fnType.arg)
		else
			fnType.arg.type = UnitType

		this.body.check scope
		this.body.unify fnResult
		this.type = fnType

export MakeObjectExec = inherit Executable
	field baseClause = null
	field method values = array()
	field method assigns = array()
	field isInstance = false
	type = UnknowableType # FIXME

	method toString = nullJoin array
		"["
		if (this.isInstance) ("New") else ("Inherit")
		this.baseClause
		"["
		join ", " (this.values)
		"]]"

	method eval = function(scope)
		let base = this.baseClause.eval(scope)
		if (base == rootObject) # Tiny optimization: Don't actually inherit from Object
			base = null
		let infields = if (base) (base.fields) else (null)
		let result = new ObjectValue(base)
		let innerScope = new ObjectValue(scope)
		innerScope.atoms.set "current" result

		if (this.isInstance && infields) # FIXME: This calls method fields even when not needed
			let i = infields.iter
			while (i.more)
				let f = i.next
				result.assign(true, f, base.apply(f))

		if (
				\>
					if (this.values) (this.values.length) else (0) # FIXME: Wait, when will this ever happen?
					if (infields) (infields.length) else (0)
			)
				fail "Tried to specify more values in \"new\" than this object has fields"

		let valueProgress = 0
		let i = this.values.iter
		while (i.more)
			let value = i.next.eval(innerScope)
			result.atoms.set (infields(valueProgress).value) value
			valueProgress = valueProgress + 1

		let i = this.assigns.iter
		while (i.more)
			let exe = i.next
			let index = null
			if (is SetExec exe)
				index = exe.indexClause.eval(innerScope) # do this early for field handling
				if (exe.isField)
					if (!is AtomLiteralExec index)
						this.fail "Objects have atom keys only"
					if (!result.fields)
						result.fields = copyArgsWithAppend(infields, index)
					else
						result.fields.append(index)
			exe.setEval(innerScope, result, index)

		if (!result.fields)
			result.fields = infields

		result

	method check = function (scope)
		let i = this.assigns.iter
		while (i.more)
			i.next.check scope

export MakeArrayExec = inherit Executable
	field contents = null
	type = UnknowableType # FIXME

	method toString = nullJoin array
		"[Array "
		join ", " (this.contents)
		"]"

	method eval = function(scope)
		let values = array()
		let i = this.contents.iter
		while (i.more)
			values.append (i.next.eval(scope))
		new ArrayValue(values)

	method check = function (scope)
		let i = this.contents.iter
		while (i.more)
			i.next.check scope

export MakeMatchExec = inherit Executable
	field matches = null
	type = UnknowableType # FIXME

	method toString = do
		let result = "[Match"
		let i = s.matches.iter
		while (i.more)
			let m = i.next
			result = result +
				nullJoin array
					" [Case "
					m.targetExe
					" ["
					join  ", " (m.unpacks)
					"] "
					m.statement.toString
					"]"
		result = result + "]"

	method eval = function(scope)
		new MatchFunctionValue(this.matches, scope)

	method check = function (scope)
		let i = this.matches.iter
		while (i.more)
			let m = i.next
			m.targetExe.check scope
			m.statement.check scope

export IfExec = inherit Executable
	field loop = false # todo isLoop
	field condClause = null
	field ifClause = null
	field elseClause = null
	field type = null

	method toString = nullJoin array
		"["
		if (this.loop) ("While") else ("If")
		" "
		this.condClause
		" "
		this.ifClause
		if (this.elseClause)
			" " + (this.elseClause.toString)
		else
			""
		"]"

	method eval = function(scope)
		if (!this.loop)
			let cond = this.condClause.eval(scope)
			if (isTrue(cond))
				if (this.ifClause)
					cond = null
					this.ifClause.eval(scope)
				else
					cond
			elif (this.elseClause)
				this.elseClause.eval(scope)
			else
				NullValue
		else
			while (isTrue(this.condClause.eval(scope)))
				this.ifClause.eval(scope)
			NullValue

	method check = function (scope)
		if (this.condClause) (this.condClause.check scope)
		this.condClause.unify (new Val(this.loc, BooleanType))
		if (this.ifClause)   (this.ifClause.check scope)
		if (this.elseClause)
			this.elseClause.check scope
			this.unify (this.ifClause)
			this.unify (this.elseClause)
		else
			this.unify (new Val(this.loc, UnitType))

export UnitExec = NullLiteralExec # Just an alias

# Values

# Util function
export isTrue = match
	NullValue = false
	FalseValue = false
	NumberValue v = (v != 0)
	_ = true

# Util function
export copyArgsWithAppend = function (ary, value)
	if (ary)
		let result = array()
		let i = ary.iter
		while (i.more)
			result.append(i.next)
		result.append value
		result
	else
		array(value)

# Util function
export toBooleanValue = function(x)
	if (x)
		TrueValue
	else
		FalseValue

# Util function
export isChild = function(parent,child)
	if (parent == child)
		true
	elif (is TrueValue child || is FalseValue child)
		parent == booleanValuePrototype
	elif (is NumberValue child)
		parent == numberValuePrototype
	elif (is StringValue child)
		parent == stringValuePrototype
	elif (is ArrayValue child)
		parent == arrayValuePrototype
	elif (is ObjectValue child)
		if (parent == rootObject)
			true
		else
			let result = false
			while (!result && child.parent)
				child = child.parent
				if (parent == child)
					result = true
			result
	else
		false

# Method constructor function
export makePrototypeApply = function(prototype, this, value)
	with value match
		AtomLiteralExec(_, key) = resolveMethod(prototype, key, this)
		_ = fail "Object has atom keys only"

export Value = inherit Object
	apply = function(value)
		fail "Apply for this object unimplemented"

export ObjectValue = inherit Value
	field parent = null
	field fields = null
	field method atoms = new Dict

	# "True" lookup function: Doesn't think about methods, keys are strings
	method innerLookup = function(key)
		if (this.atoms.has key)
			this.atoms.get key
		elif (this.parent)
			this.parent.innerLookup key
		else
			fail
				nullJoin array
					"Key not found: "
					key

	# "External" lookup function: Keys are known strings
	method lookup = function (key)
		resolveMethod this key this

	# "True" assign function: keys are strings, key must exist
	method innerAssign = function (key, value)
		if (this.atoms.has key)
			this.atoms.set key value
		elif (this.parent)
			this.parent.innerAssign key value
		else
			fail ("Tried to assign nonexistent key " + key)

	method key = function (value)
		with value match
			AtomLiteralExec(_, key) = key
			NumberValue idx = (this.fields idx).value # FIXME: Wait why are atoms stored in here?
			_ = fail "Object has atom or number keys only"

	# "External" assign function: key has no known type
	method assign = function (isLet, index, value)
		# TODO: Sanitize for atom here
		let key = this.key index
		if (isLet)
			this.atoms.set key value
		else
			this.innerAssign(key, value)

	method apply = function(index)
		this.lookup
			this.key index

export FunctionValue = inherit Value
	field argNames = null
	field exe = null
	field scope = null
	field args = null

	method apply = function(value)
		if (!this.argNames.length)
			this.exe.eval(this.scope)
		else
			let newArgs = copyArgsWithAppend(this.args, value)
			if (newArgs.length >= this.argNames.length)
				let scope = new ObjectValue(this.scope)
				let idx = 0
				while (idx < this.argNames.length)
					scope.atoms.set(this.argNames idx, newArgs idx)
					idx = idx + 1
				this.exe.eval(scope)
			else
				new FunctionValue(this.argNames, this.exe, this.scope, newArgs)

export ArrayValue = inherit Value
	field method values = array()

	method apply = function(value)
		with value match
			NumberValue number = this.values number
			AtomLiteralExec(_, key) = resolveMethod(arrayValuePrototype, key, this)
			_ = fail "Only number or atom keys allowed on array"

	method assign = function(_, index, value)
		with index match
			NumberValue number = (this.values number = value)
			_ = fail "Tried to write non-number index on array"

	method length = this.values.length # Stdlib convenience

export NullValue = inherit Value
	method apply = makePrototypeApply(nullValuePrototype, this)

export TrueValue = inherit Value
	method apply = makePrototypeApply(booleanValuePrototype, this)

export FalseValue = inherit Value
	method apply = makePrototypeApply(booleanValuePrototype, this)

export LiteralValue = inherit Value
	field value = null

export LiteralFunctionValue = inherit LiteralValue
	field count = 0
	method apply = function(value)
		let result = this.value value # Just killed tail recursion
		if (this.count < 2)
			result
		else
			new LiteralFunctionValue(result, this.count - 1)

export StringValue = inherit LiteralValue
	method apply = function(value)
		with value match
			NumberValue number = new StringValue(this.value number)
			AtomLiteralExec(_, key) = resolveMethod(stringValuePrototype, key, this)
			_ = fail "Only number or atom keys allowed on string"

	method length = this.value.length # Stdlib convenience

export NumberValue = inherit LiteralValue
	method apply = makePrototypeApply(numberValuePrototype, this)

export SuperValue = inherit Value
	field parent = null
	field target = null

	method apply = function(index)
		if (not (is AtomLiteralExec index))
			fail "Objects have atom keys only"
		resolveMethod(this.parent, index.value, this.target)

export MethodPseudoValue = inherit Object

export FunctionMethodPseudoValue = inherit MethodPseudoValue
	field scope = null
	field owner = null
	field exe = null

	method call = function(target)
		let scope = new ObjectValue(this.scope)
		scope.atoms.set "this" target
		scope.atoms.set "current" (this.owner)
		scope.atoms.set "super" new SuperValue(this.owner.parent, target)
		this.exe.eval(scope)

export LiteralMethodPseudoValue = inherit MethodPseudoValue
	field fn = null

	method call = function(target)
		this.fn.apply(target)

export resolveMethod = function(source, key, thisValue)
	let value = source.innerLookup(key)
	if (is MethodPseudoValue value)
		value.call(thisValue)
	else
		value

export MatchFunctionValue = inherit Value
	field matches = null
	field scope = null

	method apply = function(value)
		let iMatch = this.matches.iter
		let found = null
		while (!found && iMatch.more)
			let m = iMatch.next
			if (
				if (!m.targetExe)
					true
				else
					let targetValue = m.targetExe.eval(this.scope)
					isChild(targetValue, value) || equalityFilter targetValue == equalityFilter value
			)
				let scope = this.scope
				if (m.unpacks)
					scope = new ObjectValue(scope)
					let unpackIdx = 0
					while (unpackIdx < m.unpacks.length)
						let atom = m.unpacks unpackIdx
						scope.atoms.set (atom.value) (value.apply(new NumberValue(unpackIdx)))
						unpackIdx = unpackIdx + 1
				# FIXME: Interesting little quirk here: if due to a bug elsewhere this eval
				# returns a raw Python None or 0.0, very bad things will happen
				found = m.statement.eval(scope)
		if (found)
			found
		else
			fail "No match clause was met"

export globalPackageCache = new Dict

export PackageValue = inherit Value
	field tagBase = null
	field base = null
	field method loaded = new Dict

	method subpath = function(component)
		file.path.normalize
			file.path.join(this.base, component)

	method apply = function(key)
		if (!is AtomLiteralExec key)
			fail "Package has atom keys only"
		key = key.value
		if (this.loaded.has key)
			this.loaded.get key
		else
			let isDir = false
			let filename = this.subpath(key + ".em")

			if (!file.path.isFile filename)
				let dirname = this.subpath(key)
				if (file.path.isDir dirname)
					isDir = true
					filename = dirname
				else
					fail 
						nullJoin array
							"Could not find file \""
							filename
							"\" or directory \""
							dirname
							"\""

			let value = null

			if (globalPackageCache.has filename)
				value = globalPackageCache.get filename

				if (value == null)
					fail
						nullJoin array
							"File \""
							filename
							"\" attempted to recursively load itself while it was still executing"
			else
				let tag = nullJoin array(this.tagBase, ".", key)

				if (isDir)
					value = new PackageValue(tag, filename)
				else
					globalPackageCache.set filename null

					let ast = project.reader.makeAst (file.in filename, tag)
					let exe = project.parser.exeFromAst(ast)
					value = new ObjectValue()
					exe.evalSequence(defaultScope, value)

				globalPackageCache.set filename value

			this.loaded.set key value
			value

# Special objects for use in "macro" statements. Designed so calculating the macro object can be deferred
export LazyMacroLoader = inherit Value
	field cache = null

	method apply = function(key)
		if (!is AtomLiteralExec key || key.value != macroExportList)
			fail "Internal error: Bad lookup on macro object"
		if (!this.cache)
			this.cache = this.value
		this.cache

	importObject = null

let LazyMacroLambdaLoader = inherit LazyMacroLoader
	field fn = null

	method value = do
		let cache = this.fn()
		this.fn = null
		cache

let PackageAliasValue = inherit LazyMacroLoader
	field path = null
	field objectCache = null
	
	method value = do
		let cache = libraryPackage
		let i = this.path.iter
		while (i.more)
			cache = cache.apply(new AtomLiteralExec(null, i.next))
		this.path = null
		this.objectCache = cache
		cache = cache.lookup macroExportList
		cache

	method importObject = do
		if (!this.cache)
			this.cache = this.value
		this.objectCache

# Stdlib

# Util function
export literalMethod = function(f, n)
	new LiteralMethodPseudoValue(new LiteralFunctionValue(f,n))

# Used by SequenceExec

export scopeExportList = new ObjectValue
export macroExportList = new ObjectValue

# Stdlib: Builtin types

export nullValuePrototype = new ObjectValue

nullValuePrototype.atoms.set "toString"
	new StringValue("null")

export booleanValuePrototype = new ObjectValue

booleanValuePrototype.atoms.set "toString"
	literalMethod
		function (this)
			new StringValue
				if (this == FalseValue)
					"false"
				else
					"true"
		1

booleanValuePrototype.atoms.set "toNumber"
	literalMethod
		function (this)
			new NumberValue
				if (this == FalseValue)
					0
				else
					1
		1

export numberValuePrototype = new ObjectValue

numberValuePrototype.atoms.set "toString"
	literalMethod
		function (this)
			new StringValue(this.value.toString)
		1

numberValuePrototype.atoms.set "toNumber"
	literalMethod
		function (this) (this)
		1

export stringValuePrototype = new ObjectValue

stringValuePrototype.atoms.set "length"
	literalMethod
		function (this)
			new NumberValue(this.value.length)
		1

stringValuePrototype.atoms.set "toNumber"
	literalMethod
		function (this)
			new NumberValue(this.value.toNumber)
		1

stringValuePrototype.atoms.set "toString"
	literalMethod
		function (this) (this)
		1

export atomValuePrototype = new ObjectValue

atomValuePrototype.atoms.set "toString"
	literalMethod
		function (this)
			new StringValue (this.value)
		1

# Stdlib: Arrays

export arrayValuePrototype = new ObjectValue

arrayValuePrototype.atoms.set "length"
	literalMethod
		function (this)
			new NumberValue(this.values.length)
		1

arrayValuePrototype.atoms.set "append"
	literalMethod
		function (this, value)
			this.values.append value
		2

arrayValuePrototype.atoms.set "pop"
	literalMethod
		function (this)
			this.values.pop
		1

# Stdlib: Iterators

export iteratorPrototype = new ObjectValue

export IteratorObjectValue = inherit ObjectValue
	parent = iteratorPrototype
	field source = null
	field idx = 0

iteratorPrototype.atoms.set "more"
	literalMethod
		function (this)
			toBooleanValue
				this.idx < this.source.length
		1

iteratorPrototype.atoms.set "next"
	literalMethod
		function (this)
			let value = this.source.apply(new NumberValue(this.idx))
			this.idx = this.idx + 1
			value
		1

export iteratorReversePrototype = new ObjectValue

export IteratorReverseObjectValue = inherit ObjectValue
	parent = iteratorReversePrototype
	field source = null
	field idx = 0

iteratorReversePrototype.atoms.set "more"
	literalMethod
		function (this)
			toBooleanValue
				this.idx > 0
		1

iteratorReversePrototype.atoms.set "next"
	literalMethod
		function (this)
			this.idx = this.idx - 1
			this.source.apply(new NumberValue(this.idx))
		1

export installIter = function(prototype)
	prototype.atoms.set "iter"
		literalMethod
			function (this)
				new IteratorObjectValue(source = this)
			1
	prototype.atoms.set "reverseIter"
		literalMethod
			function (this)
				new IteratorReverseObjectValue(source = this, idx = this.length)
			1
installIter arrayValuePrototype
installIter stringValuePrototype

# Stdlib: File I/O

export infilePrototype = new ObjectValue

export outfilePrototype = new ObjectValue

export FileObjectValue = inherit ObjectValue
	field handle = null

export fileObject = new ObjectValue
do
	let makeFileConstructor = function(fn, prototype)
		fileObject.atoms.set (fn.toString)
			new LiteralFunctionValue
				function (path)
					new FileObjectValue(prototype, handle = file fn (path.value))
				1

	makeFileConstructor(.in,     infilePrototype)
	makeFileConstructor(.out,    outfilePrototype)
	makeFileConstructor(.append, outfilePrototype)

export closeWrapper = literalMethod
	function(this)
		this.handle.close
	1

do
	let addWrapper = function(fn)
		outfilePrototype.atoms.set (fn.toString)
			literalMethod
				function (this)
					wrapPrintRepeat (this.handle fn)
				1

	addWrapper .write
	addWrapper .print
	addWrapper .println

outfilePrototype.atoms.set "flush"
	literalMethod
		function(this)
			this.handle.flush
		1

outfilePrototype.atoms.set "close" closeWrapper

# Util function
export newString = function (x) (new StringValue (x))

do
	let addWrapper = function(fn, constructor)
		infilePrototype.atoms.set (fn.toString)
			literalMethod
				function (this)
					constructor(this.handle fn)
				1

	addWrapper .more toBooleanValue
	addWrapper .peek newString
	addWrapper .next newString

infilePrototype.atoms.set "close" closeWrapper

# Stdlib: Paths

export pathObject = new ObjectValue
fileObject.atoms.set "path" pathObject

pathObject.atoms.set "join"
	new LiteralFunctionValue
		function(x,y)
			new StringValue( file.path.join( x.value, y.value ) )
		2

do
	let addWrapper = function(fn, constructor)
		pathObject.atoms.set (fn.toString)
			new LiteralFunctionValue
				function (str)
					constructor( file.path fn (str.value) )
				1

	addWrapper .isDir toBooleanValue
	addWrapper .isFile toBooleanValue
	addWrapper .normalize newString
	addWrapper .dir newString
	addWrapper .file newString
	pathObject.atoms.set "entryFile" NullValue

# Stdlib: Dict

export dictObjectDataKey = new ObjectValue
export dictPrototype = new ObjectValue

# FIXME: Not tolerant to subclassing
export dictData = function (dict)
	if (not (dict.atoms.has dictObjectDataKey))
		dict.atoms.set dictObjectDataKey
			new Dict
	dict.atoms.get dictObjectDataKey

dictPrototype.atoms.set "get"
	literalMethod
		function(dict, index)
			(dictData dict).get
				equalityFilter index
		2

dictPrototype.atoms.set "set"
	literalMethod
		function(dict, index, value)
			(dictData dict).set
				equalityFilter index
				value
		3

dictPrototype.atoms.set "has"
	literalMethod
		function(dict, index)
			toBooleanValue
				(dictData dict).has
					equalityFilter index
		2

dictPrototype.atoms.set "del"
	literalMethod
		function(dict, index)
			(dictData dict).del
				equalityFilter index
			NullValue
		2

dictPrototype.atoms.set "iter"
	literalMethod
		function (dict) # Inefficient, it is not necessary to flatten the array
			let result = new ArrayValue
			let i = (dictData dict).iter
			while (i.more)
				let key = i.next

				# Reverse equalityFilter
				if (is Number key)
					key = new NumberValue(key)
				elif (is String key)
					key = new StringValue(key)

				result.values.append(key)
			new IteratorObjectValue(source = result)
		1

# Stdlib: "String garbage"

export charObject = new ObjectValue
do
	let charFunctions = array
		.isNonLineSpace
		.isLineSpace
		.isSpace
		.isQuote
		.isOpenParen
		.isCloseParen
		.isParen
		.isDigit
		.isIdStart
		.isIdContinue

	let iFn = charFunctions.iter
	while (iFn.more)
		let fn = iFn.next
		charObject.atoms.set (fn.toString)
			new LiteralFunctionValue
				function(x)
					toBooleanValue(char fn (x.value))
				1

# Stdlib: Scope

export equalityFilter = function (value)
	with value match
		NumberValue x = x
		StringValue x = x
		_ = value

export wrapBinaryNumber = function(f)
	new LiteralFunctionValue
		function(x,y)
			new NumberValue(f (x.value) (y.value))
		2

export wrapBinaryBoolean = function(f)
	new LiteralFunctionValue
		function(x,y)
			toBooleanValue
				f (x.value) (y.value)
		2

export wrapBinaryEquality = function(f)
	new LiteralFunctionValue
		function(x,y)
			toBooleanValue
				f (equalityFilter x) (equalityFilter y)
		2

export wrapBinaryLogical = function(f)
	new LiteralFunctionValue
		function(x,y)
			toBooleanValue
				f (isTrue x) (isTrue y)
		2

export printable = function(x)
	with x match
		StringValue v = v
		NumberValue v = v
		AtomLiteralExec = x.value
		NullValue = "null" # Redundant with toString?
		TrueValue = "true"
		FalseValue = "false"
		_ = "[Unprintable]"

export wrapPrintRepeat = function(f)
	let repeat = new LiteralFunctionValue
		function (x)
			f
				printable x
			repeat
	repeat

export setEntryFile = function(filename)
	let libraryProject = new PackageValue("project", file.path.dir(file.path.normalize(filename)))
	pathObject.atoms.set "entryFile" new StringValue(filename)
	defaultScope.atoms.set "project" libraryProject
	profileScope.atoms.set "project" libraryProject

export rootObject = new ObjectValue

export defaultScope = new ObjectValue
export profileScope = new ObjectValue

defaultScope.atoms.set "Object" rootObject
defaultScope.atoms.set "String" stringValuePrototype
defaultScope.atoms.set "Boolean" booleanValuePrototype
defaultScope.atoms.set "Number" numberValuePrototype
defaultScope.atoms.set "Array"  arrayValuePrototype

defaultScope.atoms.set "null"   NullValue
defaultScope.atoms.set "true"   TrueValue
defaultScope.atoms.set "false"  FalseValue
defaultScope.atoms.set "argv"   NullValue
defaultScope.atoms.set "ln"     new StringValue(ln)

defaultScope.atoms.set "+" 
	new LiteralFunctionValue
		function (x, y)
			let v = x.value + y.value
			with x match
				StringValue = new StringValue(v)
				NumberValue = new NumberValue(v)
		2

defaultScope.atoms.set "-" (wrapBinaryNumber \-)
defaultScope.atoms.set "*" (wrapBinaryNumber \*)
defaultScope.atoms.set "/" (wrapBinaryNumber \/)
defaultScope.atoms.set "%" (wrapBinaryNumber \%)

defaultScope.atoms.set "<"  (wrapBinaryBoolean \<)
defaultScope.atoms.set "<=" (wrapBinaryBoolean \<=)
defaultScope.atoms.set ">"  (wrapBinaryBoolean \>)
defaultScope.atoms.set ">=" (wrapBinaryBoolean \>=)

defaultScope.atoms.set "==" (wrapBinaryEquality \==)
defaultScope.atoms.set "!=" (wrapBinaryEquality \!=)

defaultScope.atoms.set "and" (wrapBinaryLogical and)
defaultScope.atoms.set "or"  (wrapBinaryLogical or)
defaultScope.atoms.set "xor" (wrapBinaryLogical xor)

defaultScope.atoms.set "neg"
	new LiteralFunctionValue
		function (x)
			new NumberValue(neg(x.value))
		1

defaultScope.atoms.set "boolean"
	new LiteralFunctionValue
		function (x)
			toBooleanValue
				isTrue x
		1

defaultScope.atoms.set "not"
	new LiteralFunctionValue
		function (x)
			toBooleanValue
				not
					isTrue x
		1

defaultScope.atoms.set "nullfn"
	new LiteralFunctionValue
		function (x) (NullValue)
		1

defaultScope.atoms.set "with"
	new LiteralFunctionValue
		function (x, y) (y.apply x)
		2

defaultScope.atoms.set "is"
	new LiteralFunctionValue
		function (x,y)
			toBooleanValue(isChild(x,y))
		2

defaultScope.atoms.set "exit"
	new LiteralFunctionValue
		function (x)
			exit(x.value)
		1

defaultScope.atoms.set "fail" # Same as exit?
	new LiteralFunctionValue
		function (x)
			fail(x.value)
		1

defaultScope.atoms.set "char" charObject

defaultScope.atoms.set "print"   (wrapPrintRepeat print)
defaultScope.atoms.set "println" (wrapPrintRepeat println)
defaultScope.atoms.set "file"    fileObject
defaultScope.atoms.set "Dict"    dictPrototype
defaultScope.atoms.set "stdout"  new FileObjectValue(outfilePrototype, handle = stdout)
defaultScope.atoms.set "stderr"  new FileObjectValue(outfilePrototype, handle = stderr)
defaultScope.atoms.set "stdin"   new FileObjectValue(infilePrototype, handle = stdin)

let libraryPackage = new PackageValue
	"project"
	file.path.normalize
		file.path.join
			file.path.join
				file.path.dir(file.path.entryFile)
				".."
			"library"
do
	let libraryProject = new ObjectValue()
	defaultScope.atoms.set "package" libraryPackage
	profileScope.atoms.set "package" libraryPackage
	defaultScope.atoms.set "project" libraryProject
	profileScope.atoms.set "project" libraryProject

profileScope.atoms.set "minimal" new LazyMacroLambdaLoader
	fn = function() (project.parser.minimalMacros)
profileScope.atoms.set "default" new LazyMacroLambdaLoader
	fn = function() (project.parser.defaultMacros)
profileScope.atoms.set "shortCircuitBoolean" new LazyMacroLambdaLoader
	fn = function() (project.parser.shortCircuitBooleanMacros)
profileScope.atoms.set "experimental" new PackageAliasValue
	path = array("emily", "profile", "experimental")

# Macro support

let makeMacroChecks = function(progress, symbol)
	if (!is NumberValue progress)
		fail "Macro progress must be a number"
	if (progress.value < 0 || progress.value >= 1000)
		fail "Macro progress must be between 0 and 999 inclusive"
	if (!is StringValue symbol)
		fail "Macro symbol is not a symbol"
	
let makeSplitMacro = function(progress, symbol)
	makeMacroChecks(progress, symbol)
	new (project.parser.SplitMacro)(progress = ProgressBase.parser + progress.value, symbol = symbol.value)
defaultScope.atoms.set "splitMacro" new LiteralFunctionValue(makeSplitMacro, 2)

let makeUnaryMacro = function(progress, symbol)
	makeMacroChecks(progress, symbol)
	new (project.parser.UnaryMacro)(progress = ProgressBase.parser + progress.value, symbol = symbol.value)
defaultScope.atoms.set "unaryMacro" new LiteralFunctionValue(makeUnaryMacro, 2)

# Dubious, intentionally "undocumented"
defaultScope.atoms.set "DEBUG"
	new LiteralFunctionValue
			function (x)
				stdout.write "----\nDEBUG: "
				with x match
					NumberValue x = stdout.write("number ", x, ln)
					StringValue x = stdout.write("string ", x, ln)
					NullValue = println "null"
					_ = do
						println "object"
						DEBUG x
				print "----\n"
			1

# This is supposed to print the keys sorted, but instead relies on dict iter incidentally sorting things
export debugScopeDump = function(obj)
	let i = obj.atoms.iter
	while (i.more)
		let key = i.next
		let value = obj.atoms.get key
		if (key == macroExportList)
			stdout.write
				"[Macros: "
				value.length
				"]"
		else
			stdout.write
				key
				": "
				if (is MethodPseudoValue value)
					"[Method]"
				else
					printable value
		stdout.write(ln)
