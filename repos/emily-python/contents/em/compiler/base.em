# Simple compiler. Currently supports only c#

profile experimental

from project.util import *
from project.compiler.util import *
from project.execution import
	SequenceExec, SetExec, IfExec, VarExec, ApplyExec, ImportAllExec, MakeFuncExec
	LiteralExec, NullLiteralExec, StringLiteralExec, AtomLiteralExec
from project.type import # FIXME
	TypedNode, ReferType, UnitType, BooleanType, NumberType, StringType, AtomType, UnknowableType
	Val, KnownTypeVal, functionTypeVal, functionTypeArgIter

# Notice use of current vs this; the current version is used when matching; the this version, when constructing

export UnitVal = inherit Val
export KnownVal = inherit Val
	field value = null

export TemplateVal = inherit Val
	field arity = 0
	field fn = null # Currently assume binary
	field prefix = null

export FnVal = inherit Val
	field block = null

export invokeTemplate = function(name)
	function(a)
		name + "(" + join(", ", a) + ")"

export upgradeTemplateVal = function(dict, name, arity, fn, prefix)
	let val = dict.get name
	if (val == chainNotFound)
		fail
			"Internal error: Tried to upgrade nonexistent val " + name
	dict.set name new TemplateVal
		val.loc, val.type, arity, fn, prefix

export PartialApplyVal = inherit Val
	field fnVal = null
	field args = null
	field argTarget = null

# Unsure
export AddressableVal = inherit Val
	field id = null

	method label = "v" + this.id.toString

export Chunk = inherit Object
	field method lines = array()

export IndentChunk = inherit Chunk

let chunkBuildImpl = function (chunk, depth)
	let result = ""
	let i = chunk.lines.iter
	while (i.more)
		let line = i.next
		result = result +
			with line match
				String = indentPrefix depth + line + "\n"
				IndentChunk = chunkBuildImpl line (depth + 1)
				Chunk = chunkBuildImpl line depth
	result

export chunkBuild = function (chunk)
	chunkBuildImpl chunk 0

# BaseCompiler covers that would apply to any possible compiler
export BaseCompiler = inherit Object
	# Note: When values here are overridden, they must preserve the same types
	scope = do
		let dict = new ChainedDict
		dict.set "ln" new KnownVal
			null, StringType, "\n"
		dict.set "true" new KnownVal
			null, BooleanType, true
		dict.set "false" new KnownVal
			null, BooleanType, false
		dict.set "println"
			functionTypeVal array(NumberType, UnitType)
		dict

	# FIXME: This object seems overloaded. What is this for?
	# Hypothesis: Once this is all well typed, a Block will be something that can register variables
	Block = inherit Object

	UnitBlock = inherit (current.Block)
		field compiler = null
		field method errors = array()
		field method source = new Chunk
		field method globalScope = new ChainedDict
		field method names = new NumGenerator
		field defsChunk = null
		field mainChunk = null
		field mainFunction = null

		method main = do
			if (this.mainChunk)
				fail "Tried to generate main chunk twice"
			this.buildMain
			this.mainFunction = new (this.compiler.Function) (this, this.mainChunk)
			this.mainFunction.appendInitialBlock

		method addVar = function(loc, type, description)
			let value = new AddressableVal
				loc = loc
				type = type
				id = this.names.next
			this.compiler.buildVarInto (this.defsChunk) value description
			value

		method addLiteral = function(exe)
			new KnownVal(exe.loc, exe.resolve.type, exe.value)

		method addRawGlobal = function(s)
			this.defsChunk.lines.append(s)

	# FIXME: Rename this. It represents a host-language function not an in-language one
	Function = inherit Object
		field unit = null # All populated by UnitBlock
		field source = null
		# Overload: firstBlock

	# FIXME: Absolutely rename this
	BlockBlock = inherit (current.Block)
		field source = null # Populated by creator

	method buildBlockImpl = function(block, scope, exe)
		with exe match
			SequenceExec(_, shouldReturn, hasScope, execs) = do
				if (hasScope)
					let newScope = new ChainedDict
					newScope.set chainParent (scope)
					scope = newScope

				let finalResult = null
				let i = execs.iter
				while (i.more)
					finalResult = this.buildBlockImpl(block, scope, i.next)
					# Allow statements in sequences with side effects but no return value
					# FIXME: Functions which return a value could very well have a side effect
					if (finalResult.resolve.type == UnitType)
						block.buildStatement(finalResult)

				if (shouldReturn)
					finalResult
				else
					UnitVal
			SetExec(_, isLet) = do
				let dataVal = this.buildBlockImpl(block, scope, exe.valueClause)

				if (dataVal.resolve.type == UnitVal)
					fail "Cannot assign unit to variable" # This message sucks

				let name = exe.indexClause.value
				let assignVal = if (isLet)
					let newTarget = block.addVar (exe.loc, dataVal.type, name)
					scope.set name newTarget
					newTarget
				else
					scope.get name

				block.buildVal(assignVal, dataVal)

				UnitVal
			VarExec(_, symbol) = do
				let val = scope.get symbol
				if (val == chainNotFound)
					# Promote a "known" value
					let knownVal = this.scope.get symbol
					if (knownVal == chainNotFound)
						fail
							"Variable name not known: " + symbol # Message should include name
					block.unit.globalScope.set symbol knownVal
					if (is TemplateVal knownVal && knownVal.prefix)
						block.addRawGlobal(knownVal.prefix)
					val = knownVal
				val
			LiteralExec = block.addLiteral exe
			ApplyExec(_, fn, arg) = do # TODO: This only works with non-curried templates now, basically
				let fnVal = this.buildBlockImpl(block, scope, fn)
				let argVal = this.buildBlockImpl(block, scope, arg)

				let simplePartial = function(argTarget)
					new PartialApplyVal
						type = exe.resolve.type
						fnVal = fnVal
						args = array(argVal)
						argTarget = argTarget

				let execute = function(exeVal, args)
					let jumpBlock = block.pointer
					let afterBlock = block.appendBlock.pointer

					let typeIter = functionTypeArgIter(exeVal.type)
					let argIter = args.iter
					while (typeIter.more)
						let argType = typeIter.next
						let argVal = argIter.next
						if (argType != UnitType)
							jumpBlock.buildPushVal (argVal)

					let resultType = exe.resolve.type
					let resultVal = null
					if (resultType != UnitType)
						resultVal = afterBlock.addVar(exe.loc, resultType, null)
						afterBlock.buildPopVal (resultVal)
					jumpBlock.pushReturn afterBlock
					jumpBlock.jumpVal exeVal

					if (resultVal)
						resultVal
					else
						UnitVal					

				let possibleExecute = function(argCount)
					if (argCount > 1)
						simplePartial argCount
					else
						execute fnVal array(argVal)

				with fnVal match
					TemplateVal = simplePartial (fnVal.arity) # All templatevals are +2-arity currently
					FnVal = possibleExecute (fnVal.argVals)
					PartialApplyVal = do
						let newLen = fnVal.args.length + 1
						let targetLen = fnVal.argTarget
						if (newLen > targetLen)
							fail "Too many applications on function for current compiler"
						let args = catArrayElement(fnVal.args, argVal)
						let innerFnVal = fnVal.fnVal
						if (newLen < targetLen || is TemplateVal innerFnVal)
							new PartialApplyVal
								type = exe.resolve.type
								fnVal = innerFnVal
								args = args
						else
							execute innerFnVal args
					_ = possibleExecute (iterCount (functionTypeArgIter (fnVal.type)))
						
			IfExec(_, loop, condClause, ifClause, elseClause) =
				if (loop)
					let jumpBlock = block.pointer
					let testBlock = block.appendBlock.pointer
					let bodyBlock = block.appendBlock.pointer
					let postBlock = block.appendBlock.pointer
					# FIXME: Assert no else block?

					jumpBlock.jump testBlock

					let condVal = this.buildBlockImpl(testBlock, scope, condClause)
					testBlock.condJump condVal bodyBlock postBlock

					this.buildBlockImpl(bodyBlock, scope, ifClause)
					bodyBlock.jump testBlock

					UnitVal
				else
					let condVal = this.buildBlockImpl(block, scope, condClause)
					let jumpBlock = block.pointer
					let ifBlock = block.appendBlock.pointer
					let elseBlock = if (elseClause) (block.appendBlock.pointer) else (null)
					let postBlock = block.appendBlock.pointer
					let type = exe.resolve.type
					let returns = type != UnitType # Implies elseClause
					let resultVar = if (returns) (jumpBlock.addVar(null, type, null)) else (UnitVal)

					jumpBlock.condJump condVal ifBlock (if (elseBlock) (elseBlock) else (postBlock))

					let tempVal = this.buildBlockImpl(ifBlock, scope, ifClause)
					if (returns)
						ifBlock.buildVal resultVar tempVal
					ifBlock.jump postBlock

					if (elseClause)
						tempVal = this.buildBlockImpl(elseBlock, scope, elseClause)
						if (returns)
							elseBlock.buildVal resultVar tempVal
						elseBlock.jump postBlock

					resultVar # Done

			MakeFuncExec(loc, args, body, funcType, typeScope) = do
				let fnScope = scope
				if (args.length > 0)
					fnScope = new ChainedDict
					fnScope.set chainParent scope

				let fnVal = new FnVal
					loc=loc, type=funcType
					block = block.appendFunctionBlock
				
				let resultType = funcType.arg.resolve.type # FIXME: Bad type assumption
				let fnBlock = fnVal.block

				let i = args.reverseIter
				while (i.more)
					let argName = i.next
					let argType = (typeScope.get argName).resolve.type
					let argVar = fnBlock.addVar (loc, argType, argName)
					fnBlock.buildPopVal(argVar)
					fnScope.set argName argVar
					argVar

				let resultVal = this.buildBlockImpl(fnBlock, fnScope, body)

				if (resultType != UnitType)
					fnBlock.buildPushVal resultVal

				fnBlock.popJump # FIXME: This assumes in *no* implementation do value stack and return stack coincide

				fnVal

			ImportAllExec = UnitVal # TODO
			NullLiteralExec = new KnownVal (value = null)

	method buildBlock = function(seqExe)
		let unit = new (this.UnitBlock) (compiler = this)
		let scope = new ChainedDict
		scope.set chainParent (unit.globalScope)

		let main = unit.main
		this.buildBlockImpl main scope seqExe
		main.terminate

		chunkBuild (unit.source)

	method build = function(exe)
		exe.check (this.scope)
		this.buildBlock exe

# ClikeCompiler is for algol-y looking languages that have a switch statement
export ClikeCompiler = inherit BaseCompiler
	scope = do
		let dict = new ChainedDict
		dict.set chainParent (current.scope)
		dict

	Function = inherit (current.Function)
		method field cases = new NumGenerator
		method field caseChunk = new Chunk

		method appendInitialBlock = do
			let block = new (this.unit.compiler.SwitchPointerBlock)
				fn = this
				block = this.appendBlock
			this.source.lines.append (this.buildEntryChunk) # Is this order bad?
			block

		method appendBlock = do
			let block = new (this.unit.compiler.SwitchBlock)
				unit = this.unit
				id = this.cases.next
			this.caseChunk.lines.append
				block.buildContentChunk
			block

	SwitchPointerBlock = inherit (current.Block)
		field fn = null
		field block = null

		method pointer = new (this.fn.unit.compiler.SwitchPointerBlock)
			fn = this.fn
			block = this.block

		method appendBlock = do
			this.block = this.fn.appendBlock
			this

		method appendFunctionBlock = new (this.fn.unit.compiler.SwitchPointerBlock)
			fn = this.fn
			block = this.fn.appendBlock

		method buildVal = this.block.buildVal
		method buildStatement = this.block.buildStatement
		method buildEntryChunk = this.block.buildEntryChunk
		method addVar = this.block.addVar
		method addLiteral = this.block.addLiteral
		method addRawGlobal = this.block.addRawGlobal
		method unit = this.block.unit
		method id = this.block.id
		method label = this.block.label
		method jump = this.block.jump
		method jumpVal = this.block.jumpVal
		method condJump = this.block.condJump
		method terminate = this.block.terminate
		method pushReturn = this.block.pushReturn
		method popJump = this.block.popJump
		method buildPushVal = this.block.buildPushVal
		method buildPopVal = this.block.buildPopVal

	SwitchBlock = inherit (current.BlockBlock)
		field id = null

		method buildVal = function(assignVal, dataVal)
			if (!assignVal)
				assignVal = this.addVar(null, dataVal.type, null)
			let compiler = this.unit.compiler
			appendArray (this.source.lines) array
				compiler.valToString(assignVal) + " = " + compiler.valToString(dataVal) + ";"

		method buildStatement = function(val)
			appendArray (this.source.lines) array
				this.unit.compiler.valToString(val) + ";"

		method addVar = this.unit.addVar
		method addLiteral = this.unit.addLiteral
		method addRawGlobal = this.unit.addRawGlobal

		method label = this.id.toString

		# These just happen to be the same in all subclasses
		standardExitLines = array
			"break;"

		standardTerminateLines = array
			"run = false;"
			"break;"

		iLine = function(block)
			"i = " + block.label + ";"

		field method exitChunk = new Chunk # The contents of this get modified. Is that "okay?"
			lines = this.standardExitLines

		method buildContentChunk = do
			this.source = new Chunk
			new IndentChunk
				lines = array
					"case " + this.label + ": {"
					new IndentChunk
						lines = array
							this.source
							this.exitChunk
					"}"

		method jumpVal = function(val)
			if (is FnVal val)
				fail "Okay to remove this fail, just tracing: Jumped to a val directly"
				this.jump (val.block)
			else
				this.exitChunk.lines = this.standardExpressionStringJump
					this.unit.compiler.valToString val

		method terminate = do
			this.exitChunk.lines = this.standardTerminateLines

		method standardCondJumpArray = function(condString, trueBlock, falseBlock)
			array
				"if (" + condString + ")"
				new IndentChunk
					lines = array
						"i = " + trueBlock.label + ";"
				"else"
				new IndentChunk
					lines = array
						"i = " + falseBlock.label + ";"
				"break;"

		method standardExpressionStringJump = function(str)
			array
				"i = " + str + ";"
				"break;"

		method standardFallthroughJump = function(block)
			this.exitChunk.lines = 
				if (block.id == this.id + 1)
					array()
				else
					array
						this.iLine block
						"break;"

		method standardFallthroughCondJump = function(condVal, trueBlock, falseBlock)
			let condString = this.unit.compiler.valToString condVal
			this.exitChunk.lines = 
				if (falseBlock.id == this.id + 1)
					array
						"if (" + condString + ") {"
						new IndentChunk
							lines = array
								this.iLine trueBlock
								"break;"
						"}"
				elif (trueBlock.id == this.id + 1)
					array
						"if (!(" + condString + ")) {"
						new IndentChunk
							lines = array
								this.iLine falseBlock
								"break;"
						"}"
				else
					this.standardCondJumpArray(condString, trueBlock, falseBlock)

	method valToString = function(val)
		with val match
			AddressableVal = val.label
			KnownVal = this.literalToString (val.value)
			PartialApplyVal = do
				let fnVal = val.fnVal
				with fnVal match
					TemplateVal = do
						if (val.args < fnVal.arity)
							fail "No currying yet"
						fnVal.fn (map (this.valToString) (val.args))
			FnVal = val.block.label

	method literalToString = function(value)
		with value match
			String = "\"" + value + "\"" # NO!
			Number = value.toString + "f"
			Boolean = value.toString
			null = "null"
			_ = fail "Can't translate this literal"


# Mass-install binary operators which conveniently are the same in Emily + All targets
do
	let bScope = BaseCompiler.scope
	let cScope = ClikeCompiler.scope
	let install = function (ops, returnType)
		let i = ops.iter
		while (i.more)
			let name = i.next
			let loc = null
			bScope.set name
				functionTypeVal array(NumberType, NumberType, returnType)
			upgradeTemplateVal
				cScope, name, 2
				function(a)
					"(" + a 0 + ") " + name + " (" + a 1 + ")"
				null
	install array("+", "-", "*", "/", "%") NumberType # FIXME: "%" inappropriate for C++
	install array("<=", ">=", "<", ">", "==") BooleanType

# CtypedCompiler is methods common to C and C# (ie explicitly typed languages) but not JavaScript
export CtypedCompiler = inherit ClikeCompiler
	UnitBlock = inherit (current.UnitBlock)
		method buildMain = do
			this.defsChunk = new Chunk
			this.mainChunk = new IndentChunk

	