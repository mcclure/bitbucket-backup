# Parser: Transformations ("macros") applied to parse tree to gradually make it executable

profile experimental

from project.util import *
from project.core import *
from project.reader import
	SymbolExp, QuoteExp, NumberExp, ExpGroup
from project.execution import
	UnitExec, StringLiteralExec, AtomLiteralExec, NumberLiteralExec,
	NullLiteralExec, BooleanLiteralExec, StoredLiteralExec
	InvalidExec, VarExec, ApplyExec, SetExec, IfExec, SequenceExec, ImportAllExec
	MakeFuncExec, MakeMatchExec, MakeArrayExec, MakeObjectExec
	ObjectValue, UserMacroList, LazyMacroLoader, macroExportList, profileScope, defaultScope

# Base/helper

export Macro = inherit Object
	progress = ProgressBase.reader

export insertMacro = insertLinked function(x,y)
	cmp (x.progress) (y.progress)

export isSymbol = function(node, goal)
	with node match
		SymbolExp = !node.isAtom && !node.isEscaped && node.content == goal
		_ = false

export stripLeftSymbol = function(list, goal)
	if (not (list.length))
		null
	else
		let left = list 0
		if (isSymbol(left, goal))
			popLeft list
			left
		else
			null

export SequenceTracker = inherit Object
	field statements = null
	field idx = 0

	method more = this.statements.length > this.idx
	method next = do
		let result = this.statements (this.idx)
		this.idx = this.idx + 1
		result

	method steal = function(symbol)
		if (this.more)
			let nodes = this.statements(this.idx).nodes
			if (nodes && isSymbol (nodes 0) symbol)
				this.next
				nodes
			else
				null
		else
			null

export getNextExp = function(parser, loc, isExp, symbol, ary)
	let failMsg = function(a) (parser.error(loc, nullJoin a))

	if (!ary.length)
		failMsg array
			"Emptiness after \""
			symbol
			"\""
	elif (isExp && !is ExpGroup (ary 0))
		failMsg array
			"Expected a (group) after \""
			symbol
			"\""
	else
		popLeft ary

# Macro classes

# FIXME: In which places is null allowed in place of array()? In which places *should* it be?

# Abstract macro: Matches on a single known symbol
export OneSymbolMacro = inherit Macro
	method matches = function(left, node, right)
		isSymbol(node, this.symbol)

# User requesting macro load
export MacroMacro = inherit OneSymbolMacro
	field isProfile = false
	progress = ProgressBase.parser + 10

	method symbol = if (this.isProfile) ("profile") else ("macro")

	method apply = function(parser, left, node, right, _)
		let isExport = false
		let error = null

		if (left.length)
			if (!(left.length == 1) && isSymbol(left 0, "export"))
				error = parser.error(node.loc, "Stray garbage before \"" + this.symbol + "\"")
			isExport = true

		if (!right.length)
			error = parser.error(node.loc, "Emptiness after \"" + this.symbol + "\"")

		if error
			error
		else
			let macroGroup = right 0
			let result = null

			if (is SymbolExp macroGroup)
				# TODO: Consider only allowing this if atom keys. TODO: Do something more sensible when this fails?
				let macroObject = parser.process(node.loc, right, null).eval(profileScope)
				# TODO: This can fail
				let macroList = macroObject.apply(new AtomLiteralExec(node.loc, macroExportList))

				if (!is Array macroList)
					error = parser.error(macroGroup.loc, "macro import path did not resolve to a valid module")
				else
					let payload = null
					let importObject = macroObject

					if (is LazyMacroLoader importObject)
						importObject = importObject.importObject

					if importObject
						payload = new ImportAllExec
							node.loc
							new StoredLiteralExec(node.loc, importObject)
							isExport

					result = new ProcessResult
							null
							new UserMacroList(node.loc, macroList, isExport, this.isProfile, payload)
							null

			elif (is ExpGroup macroGroup)
				if (right.length > 1)
					error = parser.error(node.loc, "Stray garbage after \"" + this.symbol + " (group)\"")
				let macroExecs = parser.makeArray(macroGroup)
				let macroValues = array()
				let i = macroExecs.iter
				while (i.more)
					macroValues.append
						i.next.eval(defaultScope)
				result = new ProcessResult
					null
					new UserMacroList(node.loc, macroValues, isExport, this.isProfile)
					null

			else
				error = parser.error(node.loc, u"Expected a path or a (group) after \"" + this.symbol + "\"")

			if error (error) else (result)

# Abstract macro: Expects KNOWNSYMBOL (GROUP)
export SeqMacro = inherit OneSymbolMacro
	method apply = function(parser, left, node, right, _)
		let exp = getNextExp(parser, node.loc, true, this.symbol, right)

		if (is InvalidExec exp)
			exp
		else
			new ProcessResult(left, this.construct(parser, exp), right)

# from a import b
export ImportMacro = inherit OneSymbolMacro
	progress = ProgressBase.parser + 10
	symbol = "import"

	method generateSetExec = function(parser, loc, prefix, target)
		if (!target.length)
			parser.error(loc, "Missing target to import")
		else
			let error = null

			if (prefix)
				let targetStart = target 0
				if (is SymbolExp targetStart)
					if (targetStart.isAtom)
						error = parser.error(targetStart.loc, "Expected a symbol after \"import\"")
					else
						target = cloneArray target
						target 0 = new SymbolExp(loc, targetStart.content, true)
				if (not error)
					target = catArray prefix target

			if (!error && target.length == 1)
				error = parser.error(target(0).loc, "import expects either multiple symbols or a \"from\" clause")

			if (not error)
				let targetEnd = lastFrom target
				if (is SymbolExp targetEnd && !targetEnd.isAtom)
					error = parser.error(targetEnd.loc, "End of import path needs to be an atom")

			if (error)
				error
			else
				let targetEnd = lastFrom target
				let symbol = new AtomLiteralExec(targetEnd.loc, targetEnd.content)
				
				new SetExec(loc, true, false, false, false, null, symbol, parser.process(loc, target, null))

	method apply = function(parser, left, node, right, tracker)
		let prefix = null
		let error = null

		if (left.length)
			if (isSymbol(left 0, "from"))
				if (left.length == 1)
					error = parser.error(left(0).loc, "Expected symbols between \"from\" and \"import\"")
				else
					prefix = cloneArray left
					popLeft prefix
			else
				error = parser.error(node.loc, "Stray garbage before \"import\"")

		if (error)
			error
		else
			let result = null

			if (right.length == 1 && is ExpGroup (right 0))
				let setExecs = array()
				let i = right(0).statements.iter
				while (!result && i.more)
					let setExec = this.generateSetExec(parser, node.loc, prefix, i.next.nodes)
					if (is Error setExec)
						result = setExec
					else
						setExecs.append setExec
				if (!result)
					result = new SequenceExec(node.loc, false, false, setExecs)
			elif (right.length == 1 && isSymbol (right 0) "*")
				result = new ImportAllExec(node.loc, parser.process(node.loc, prefix, null))
			else
				result = this.generateSetExec(parser, node.loc, prefix, right)

			if (is Error result)
				result
			else
				new ProcessResult(null, result, null)

# a = b
export SetMacro = inherit OneSymbolMacro
	progress = ProgressBase.parser + 100
	symbol = "="

	method apply = function(parser, left, node, right, tracker) # DO I NEED TO COPY RIGHT?
		let exec = new SetExec(node.loc)

		let pending = true
		while (pending)
			if (stripLeftSymbol(left, "let"))
				exec.isLet = true
			elif (stripLeftSymbol(left, "field"))
				exec.isField = true
			elif (stripLeftSymbol(left, "method"))
				exec.isMethod = true
			elif (stripLeftSymbol(left, "export"))
				exec.isExport = true
			else
				pending = false

		if (exec.isLet && exec.isExport)
			parser.error(node.loc, "Cannot use \"let\" and \"export\" together")
		if (!left.length)
			parser.error(node.loc, "Missing name in =")
		else
			let process = parser.process(node.loc)
			let index = left.pop
			let failedAt = null
			if (left.length)
				exec.targetClause = process(left, null) 
				exec.indexClause = process(array (index), null)
			else
				if (is SymbolExp index && !index.isAtom)
					exec.indexClause = new AtomLiteralExec(index.loc, index.content)
				else
					failedAt = index.loc

			if (failedAt)
				parser.error(failedAt, "Assigned name must be alphanumeric")
			else
				exec.valueClause = process(right, tracker)

				new ProcessResult(null, exec, null)				

# do (statements)
export DoMacro = inherit SeqMacro
	progress = ProgressBase.parser + 400
	symbol = "do"

	method construct = function(parser, seq)
		parser.makeSequence(seq.loc, seq.statements, true)

# function(args) (body)
export FunctionMacro = inherit OneSymbolMacro
	progress = ProgressBase.parser + 400
	symbol = "function"

	method apply = function(parser, left, node, right, _)
		let getNextGroup = getNextExp(parser, node.loc, true)

		let argExp = getNextGroup(this.symbol, right)
		if (is InvalidExec argExp)
			argExp
		else
			let bodyExp = getNextGroup(this.symbol + " (args)", right)
			if (is InvalidExec bodyExp)
				bodyExp
			else
				let args = array()
				let argError = null
				if (!argExp.empty)
					let i = argExp.statements.iter
					while (and (not argError) (i.more))
						let stm = i.next
						let failBecause = function(reason)
							argError = parser.error
								node.loc
								nullJoin array
									"Arg #"
									args.length
									" on "
									this.symbol
									" is "
									reason

						if (!stm.nodes.length)
							failBecause "blank"
						elif (stm.nodes.length !=  1)
							failBecause "an expression"
						elif (!is SymbolExp (stm.nodes 0))
							failBecause "not a symbol"
						else
							args.append(stm.nodes(0).content)

				if (argError)
					argError
				else
					new ProcessResult
						left
						new MakeFuncExec
							node.loc
							args
							parser.makeSequence(bodyExp.loc, bodyExp.statements, true)
						right

export IfMacro = inherit OneSymbolMacro
	field loop = false
	
	progress = ProgressBase.parser + 400

	method symbol = if (this.loop) ("while") else ("if")

	method apply = function(parser, left, node, right, tracker)
		let getNext = getNextExp(parser, node.loc)

		let condExp = getNext(false, this.symbol, right)
		if (is InvalidExec condExp)
			condExp
		else
			let seqExp = getNext(true, this.symbol + " (group)", right)

			if (is InvalidExec seqExp)
				seqExp
			else
				let condExec = parser.process(condExp.loc, array(condExp), null)
				let seqExec = parser.makeSequence(seqExp.loc, seqExp.statements, not (this.loop))
				let elseExec = null

				if (not (this.loop))
					if (!nonempty right && tracker)
						right = tracker.steal "else"
					if (!nonempty right && tracker)
						right = tracker.steal "elif"
					if (nonempty right)
						if (isSymbol(right 0, "else"))
							popLeft(right)
							let elseExp = getNext(true, "else", right)
							if (elseExp)
								elseExec = parser.makeSequence(elseExp.loc, elseExp.statements, true)
						elif (isSymbol(right 0, "elif"))
							let elifSymbol = popLeft(right)
							let elseResult = this.apply(parser, array(), elifSymbol, right, tracker)
							if (is ProcessResult elseResult)
								elseExec = elseResult.at
								right = elseResult.right
							else
								elseExec = elseResult

				if (is InvalidExec elseExec)
					elseExec
				else
					new ProcessResult(left, new IfExec(node.loc, this.loop, condExec, seqExec, elseExec), right)

export MatchCase = inherit Object
	field targetExe = null
	field unpacks = null
	field statement = null

export MatchMacro = inherit OneSymbolMacro
	progress = ProgressBase.parser + 400
	symbol = "match"

	method apply = function (parser, left, node, right, tracker)
		let exp = getNextExp(parser, node.loc, true, this.symbol, right)

		if (is InvalidExec exp)
			exp
		else
			let result = array()
			let iStm = exp.statements.iter
			let foundError = null

			while (iStm.more)
				let stm = iStm.next
				if (stm.nodes.length)
					let eqNode = null
					let eqLeft = array()
					let eqRight = array()

					let iNode = stm.nodes.iter
					while (iNode.more)
						let node = iNode.next
						if (eqNode)
							eqRight.append node
						elif (isSymbol node "=")
							eqNode = node
						else
							eqLeft.append node

					if (not eqNode)
						foundError = parser.error(stm.nodes(0).loc, "Match line does not have an =")
					elif (not (eqLeft.length))
						foundError = parser.error(eqNode.loc, "Left of = in match line is blank")
					elif (eqLeft.length > 2)
						foundError = parser.error(eqLeft(2).loc, "Left of = in match line has too many symbols. Try adding parenthesis?")
					elif (!eqRight.length)
						foundError = parser.error(eqNode.loc, "Right of = in match line is blank")
					else
						let targetExp = popLeft eqLeft
						let unpacksExp = null
						let unpacks = array()

						if (eqLeft.length) # There is an unpack list
							let garbled = false

							unpacksExp = eqLeft 0
							if (is SymbolExp unpacksExp)
								unpacks.append
									new AtomLiteralExec(unpacksExp.loc, unpacksExp.content)
							elif (is ExpGroup unpacksExp)
								let iUnpack = unpacksExp.statements.iter
								while (!garbled && iUnpack.more)
									let unpackStatement = iUnpack.next
									if (!unpackStatement.nodes.length)
										garbled = true
									else
										let unpackSymbol = unpackStatement.nodes 0
										if (!is SymbolExp unpackSymbol)
											garbled = true
										else
											unpacks.append(new AtomLiteralExec(unpackSymbol.loc, unpackSymbol.content))
							else
								garbled = true # Technically redundant

							if (garbled || !unpacks.length)
								foundError = parser.error(unpacksExp.loc, "In match line, variable unpack list on left of = is garbled")

						if (!foundError)
							if (isSymbol targetExp "_")
								if (unpacksExp)
									foundError = parser.error(unpacksExp.loc, "In match line, variable unpack list used with _")
								else
									targetExp = null # Null denotes wildcard match
							elif (isSymbol targetExp "array")
								if (!unpacksExp)
									foundError = parser.error(unpacksExp.loc, "In match line, variable unpack list missing after \"array\"")
								else
									targetExp = null

						if (!foundError)
							let targetExec = 
								if (targetExp)
									parser.process (targetExp.loc, array (targetExp), null)
								else
									null
							let tempStatement = parser.process(eqRight(0).loc, eqRight, tracker)
							result.append(new MatchCase(targetExec, unpacks, tempStatement))

			if (foundError)
				foundError
			else
				new ProcessResult (left, new MakeMatchExec(node.loc, result), right)

export ArrayMacro = inherit SeqMacro
	progress = ProgressBase.parser + 500
	symbol = "array"

	method construct = function(parser, seq)
		new MakeArrayExec(seq.loc, parser.makeArray(seq))

export ObjectMacro = inherit OneSymbolMacro
	field instance = false

	progress = ProgressBase.parser + 500
	
	method symbol = if (this.instance) ("new") else ("inherit")

	method apply = function(parser, left, node, right, _)
		let getNext = getNextExp(parser, node.loc)
		let baseExp = getNext(false, this.symbol, right)
		if (is InvalidExec baseExp)
			baseExp
		else
			let baseExec = parser.process(baseExp, array(baseExp), null)

			let seqExp = if (!right.length)
				new ExpGroup(baseExp.loc)
			else
				getNext(true, this.symbol + " [base]", right)

			if (is InvalidExec seqExp)
				seqExp
			else
				let seq = if (not (seqExp.empty))
					parser.makeSequence(seqExp.loc, seqExp.statements, false).execs
				else
					array()

				let values = array()
				let assigns = array()
				let foundSet = false
				let foundError = null

				let i = seq.iter
				while (!foundError && i.more)
					let assign = i.next

					if (is SetExec assign)
						foundSet = true
						if (assign.targetClause)
							foundError = parser.error(assign.loc, "Assignment inside object literal was not of form key=value")
						elif (assign.isLet || assign.isExport)
							foundError = parser.error(assign.loc, "Found a stray value expression inside an object literal")
						else
							assign.isLet = true
							assigns.append assign
					elif (is ImportAllExec assign)
						foundSet = true
						assigns.append assign
					else
						if foundSet
							foundError = parser.error(assign.loc, "Found a stray value expression inside an object literal")
						else
							values.append assign

				if (foundError)
					foundError
				else
					new ProcessResult(left, new MakeObjectExec(node.loc, baseExec, values, assigns, this.instance), right)

export ValueMacro = inherit Macro
	progress = ProgressBase.parser + 900

	method matches = function(left, node, right)
		with node match
			QuoteExp = true
			NumberExp = true
			SymbolExp = true
			_ = false

	method apply = function(parser, left, node, right, _)
		node = with node match
			QuoteExp(loc, content) = new StringLiteralExec(loc, content)
			NumberExp(loc, integer, dot, decimal) = do
				let value = integer
				if (dot)
					value = value + "."
				if (decimal)
					value = value + decimal
				new NumberLiteralExec(loc, value.toNumber)
			SymbolExp(loc, content, isAtom) = do
				if (isAtom)
					new AtomLiteralExec(loc, content)
				else
					new VarExec(loc, content)
			_ = parser.error(node.loc, "Internal error: AST node of indecipherable type found at end of macro processing")

		if (is InvalidExec node)
			node
		else
			new ProcessResult(left, node, right)

export FancySplitterMacro = inherit OneSymbolMacro
	method apply = function(parser, left, node, right, tracker)
		if (!left)
			parser.error(node.loc, "Emptiness after \"" + node.content + "\"")
		elif (!right)
			parser.error(node.loc, u"Emptiness after \"" + node.content + "\"")
		else
			let leftExe = parser.process(node.loc, left, null)
			let rightExe = parser.process(node.loc, right, tracker)
			this.expression(node.loc, leftExe, rightExe)

export AndMacro = inherit FancySplitterMacro
	progress = ProgressBase.parser + 605
	symbol = "&&"

	method expression = function(loc, leftExe, rightExe)
		let trueHere = new BooleanLiteralExec(loc, true)
		let falseHere = new BooleanLiteralExec(loc, false)
		new ProcessResult
			null
			new IfExec
				loc, false, leftExe
				new IfExec(loc, false, rightExe, trueHere, falseHere)
				falseHere
			null

export OrMacro = inherit FancySplitterMacro
	progress = ProgressBase.parser + 603
	symbol = "||"

	method expression = function(loc, leftExe, rightExe)
		let trueHere = new BooleanLiteralExec(loc, true)
		let falseHere = new BooleanLiteralExec(loc, false)
		new ProcessResult
			null
			new IfExec
				loc, false, leftExe
				trueHere
				new IfExec(loc, false, rightExe, trueHere, falseHere)
			null

# User defined macro constructors
export UserMacro = inherit OneSymbolMacro

export SplitMacro = inherit UserMacro
	method apply = function(parser, left, node, right, tracker)
		if (!left.length) # Slight code redundancy with FancySplitter?
			parser.error(node.loc, "Emptiness after \"" + node.content + "\"")
		elif (!right.length)
			parser.error(node.loc, "Emptiness after \"" + node.content + "\"")
		else
			new ProcessResult
				null
				new ApplyExec
					node.loc
					new ApplyExec
						node.loc
						new VarExec(node.loc, this.symbol)
						parser.process(node.loc, left, null)
					parser.process(node.loc, right, tracker)
				null

export UnaryMacro = inherit UserMacro
	method apply = function(parser, left, node, right, tracker)
		if (!right)
			parser.error(node.loc, u"Emptiness after \"" + this.symbol + "\"")
		else
			new ProcessResult
				left
				new ApplyExec
					node.loc
					new VarExec(node.loc, this.symbol)
					parser.process(node.loc, right, tracker)
				null

# Standard macro sets

export minimalMacros = array
	SetMacro
	ValueMacro

export defaultMacros = catArray minimalMacros array
	new MacroMacro (false)
	new MacroMacro (true)
	ImportMacro
	DoMacro
	FunctionMacro
	new IfMacro( false )
	new IfMacro( true )
	MatchMacro
	ArrayMacro
	new ObjectMacro ( false )
	new ObjectMacro ( true )

export shortCircuitBooleanMacros = array
	OrMacro
	AndMacro

# Parser

export ProcessResult = inherit Object
	field left = null
	field at = null
	field right = null

# Used to manage Parser.process needing to be able to iterate both "left to right" and "right to left"
let BidiIterator = inherit Object
	field method source = array()
	field method result = array()
	field rightward = false

	method left =  if (this.rightward) (this.source) else (this.result)
	method right = if (this.rightward) (this.result) else (this.source)

	method push = function(v)
		if (this.rightward)
			appendLeft(this.result, v)
		else
			this.result.append(v)

	method pop = do
		if (this.rightward)
			this.source.pop
		else
			popLeft(this.source)

	method replace = function(left, at, right)
		if (!left)  (left = array())
		if (!right) (right = array())
		if (this.rightward)
			this.source = left
			this.result = right
		else
			this.source = right
			this.result = left
		if (at)
			this.push(at)

export Parser = inherit Object
	field macros = null # Linked[Macro]
	field errors = array()

	method makeSequence = function(loc, statements, shouldReturn)
		let execs = array()
		let macros = null
		let tracker = new SequenceTracker(statements)
		let parser = this
		let customMacros = false
		let hasLets = false

		while (tracker.more)
			let statement = tracker.next

			let exe = parser.process(loc, statement.nodes, tracker)
			if (is UserMacroList exe)
				if (!customMacros || exe.isExport)   # Instantiate parser on first custom macro, or any "profile" call
					parser = cloneParser(parser, exe.isProfile)
					customMacros = true
				parser.loadAll(exe.contents)
				if (exe.isExport)
					if (!macros)
						macros = array()
					appendArray(macros, exe.contents)
				if (exe.payload)
					execs.append(exe.payload)
			else
				execs.append(exe)

		let i = execs.iter
		while (!hasLets && i.more)
			let exe = i.next
			if ( is SetExec exe && (exe.isLet || exe.isExport) )
				hasLets = true

		new SequenceExec(loc, shouldReturn, hasLets, execs, macros)

	method makeArray = function(expGroup)
		let result = array()
		if (!expGroup.empty)
			let tracker = new SequenceTracker(expGroup.statements)
			while (tracker.more)
				let statement = tracker.next
				let exe = this.process(expGroup.loc, statement.nodes, tracker)
				result.append exe
		result

	method loadAll = function(macros)
		let i = macros.iter
		while (i.more)
			let add = i.next
			this.macros = insertMacro(this.macros, add)

	method error = function(loc, msg)
		this.errors.append( new Error(loc, msg) )
		new InvalidExec(loc)

	method checkComplete = function(node)
		if (node.progress < ProgressBase.executable)
			this.error(node.loc, "Macro malfunctioned and left an unfinished node here at end of processing")
		else
			null

	method process = function(loc, nodes, tracker)
		# Callers must define the semantics of empty lists themselves.
		if (nodes.length == 0)
			this.error(loc, "Internal error: Parser attempted to evaluate an empty statement. This is a bug in the interpreter.")
		else
			# First, apply all macros to the statement.
			let macroNode = this.macros
			let foundError = null

			while (macroNode && !foundError)
				let levelProgress = macroNode.value.progress
				let i = new BidiIterator(nodes, rightward = (1 == levelProgress % 2))
				
				# Iterate over tokens in BidiIterator and macro-filter on the way
				while (i.source.length && !foundError)
					let at = i.pop
					let left = i.left
					let right = i.right
					
					# All the macros of this "level" will be evaluated together for this token.
					let matched = false # Did anything happen this level?

					if (at.progress <= levelProgress) # Don't even bother if this symbol is already progressed
						let levelMacroNode = macroNode

						# Iterate over the sub-portion of the macro list with this level
						while (!matched && levelMacroNode && levelMacroNode.value.progress == levelProgress)
							let m = levelMacroNode.value # Macro to test

							if ( m.matches(left, at, right) )
								matched = true # Don't match this macro level further

								# apply() returns either a ProcessResult so processing can continue,
								# or a bare InvalidExec to signal that processing should short-circuit.
								let result = m.apply(this, left, at, right, tracker)

								# Unpack
								with result match
									InvalidExec =
										foundError = result
									ProcessResult(_left, _at, _right) = do
										left = _left
										at = _at
										right = _right

								i.replace(left, at, right)

							levelMacroNode = levelMacroNode.next

					if (!matched)
						i.push at

				# Step forward past this macro level
				if (!foundError)
					while (macroNode && macroNode.value.progress == levelProgress)
						macroNode = macroNode.next

					nodes = i.result # Done with BidiIterator

			if (foundError)
				foundError
			elif (!nodes.length)
				# TODO: Try to figure out which macro? parser.py assumes "at" is the culprit...
				this.error(loc, "Macro malfunctioned and produced an empty list")
			else
				# We now have a list of progress=Macroed symbols. Treat the list as curried applications.
				# We need to pack the list of values down into one application tree & sanitize with "checkComplete"
				let result = null       # Current "leftmost value"
				let resultError = null  # Short-circuit if a problem occurs

				# The "result" value starts as the zeroth item in the list
				if (is ExpGroup (nodes 0))
					let firstNode = popLeft nodes
					if (firstNode.empty)                   # ()
						result = new UnitExec (firstNode.loc)
					elif (firstNode.statements.length == 1) # (arg)
						result = this.process (firstNode.loc, firstNode.statements(0).nodes, null)
					else                                   # (arg1, arg2, ...)
						tracker = new SequenceTracker(firstNode.statements)
						result = this.process (firstNode.loc, tracker.next.nodes, tracker)
						if (tracker.more)
							resultError = this.error (firstNode.loc, "Line started with a multiline parenthesis group. Did you mean to use \"do\"?")
		
				else
					result = popLeft(nodes)
					resultError = this.checkComplete result # First write so this is safe

				# For each item in the list, apply result = result(b)
				while (!resultError && nodes.length)
					let arg = popLeft(nodes)

					if (is ExpGroup arg)
						if (arg.empty) # ()
							result = new ApplyExec(arg.loc, result, new UnitExec(arg.loc))
						else                    # (arg1, arg2, ...)
							let argIdx = 0 # Used by error message
							let tracker = new SequenceTracker(arg.statements)
							while (tracker.more)
								let statement = tracker.next
								argIdx = argIdx + 1
								if (!statement.nodes.length)
									resultError = this.error
										arg.loc
										nullJoin array
											"Argument #"
											argIdx
											" to function is blank"
								else
									result = new ApplyExec(arg.loc, result, this.process(arg, statement.nodes, tracker))
					else
						resultError = this.checkComplete arg
						if (!resultError)
							result = new ApplyExec(arg.loc, result, arg)

				if resultError
					resultError
				else
					result

export cloneParser = function(parser, reset)
	new Parser
		if (!reset) (cloneLinked (parser.macros))
		parser.errors

export exeFromAst = function(ast)
	let parser = new Parser
	parser.loadAll defaultMacros
	let result = parser.makeSequence(ast.loc, ast.statements, false)

	checkErrors(parser.errors)

	result
