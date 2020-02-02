-- Take one number from input, print its factors as space-separated list (pure LLVM calls)
-- Combined test of arithmetic, array indices, branches, functions, C library prototypes

-- Arg: 924
-- Expect: 2 2 3 7 11

require("lib-lua/simple")

local backend = Backend("main")
local builder = backend.builder
local module = backend.module

-- Main function prototype
local mainParams = typeArray { int32Type, llvm.pointerType(charPtrType, 0) }
local mainType = llvm.functionType(int32Type, mainParams, 2, false)
local mainFn = module.addFunction("main", mainType)

-- Printf prototype
local printfParams = typeArray { charPtrType }
local printfType = llvm.functionType(int32Type, printfParams, 1, true) -- vararg
local printfFn = module.addFunction("printf", printfType)

-- PrintFactors prototype
local printFactorsParams = typeArray { int32Type }
local printFactorsType = llvm.functionType(llvm.voidType(), printFactorsParams, 1, false)
local printFactorsFn = module.addFunction("printFactors", printFactorsType)

-- Main function implementation
local mainEntry = llvm.appendBasicBlock(mainFn, "Entry")
builder.positionBuilderAtEnd(mainEntry)

	-- Branch on argc
	local argcValid = llvm.appendBasicBlock(mainFn, "ArgcValid")
	local argcInvalid = llvm.appendBasicBlock(mainFn, "ArgcInvalid")
	local argcCmp = builder.buildICmp(llvm.intEQ, -- argc == 2
		llvm.getParam(mainFn, 0), llvm.constInt(int32Type, 2, false), "ArgcCompare")
	builder.buildCondBr(argcCmp, argcValid, argcInvalid)

	-- argc == 2 case
	builder.positionBuilderAtEnd(argcValid)
		-- Read argv[1]
		local argvIndex = valueArray { llvm.constInt(int32Type, 1, false) }
		local argvIndexAddr = builder.buildGEP(llvm.getParam(mainFn, 1), argvIndex, 1, "ArgvIndex")
		local argvValue = builder.buildLoad(argvIndexAddr, "ArgvDeref")

		-- Atoi prototype
		local atoiParams = typeArray { charPtrType }
		local atoiType = llvm.functionType(int32Type, atoiParams, 1, true)
		local atoiFn = module.addFunction("atoi", atoiType)

		-- Atoi call site
		local atoiArgs = valueArray { argvValue }
		local atoiResult = builder.buildCall(atoiFn, atoiArgs, 1, "atoicall")

		-- PrintFactors call site
		local printFactorsArgs = valueArray { atoiResult }
		builder.buildCall(printFactorsFn, printFactorsArgs, 1, "")

		-- Printf call site (newline)
		local printfArgs = valueArray { builder.buildGlobalStringPtr("\n", "printfnewline") }
		builder.buildCall(printfFn, printfArgs, 1, "printfnumbercall")

		-- Return success
		builder.buildRet(llvm.constInt(int32Type, 0, false))

	-- argc != 2 case
	builder.positionBuilderAtEnd(argcInvalid)
		-- Print error
		local errorString = "Please enter exactly one number as argument.\n"
		local printfArgs = valueArray { builder.buildGlobalStringPtr(errorString, "printferror") }
		builder.buildCall(printfFn, printfArgs, 1, "printferrorcall")

		-- Return failure
		builder.buildRet(llvm.constInt(int32Type, 1, false))

-- PrintFactors implementation
local printFactorsEntry = llvm.appendBasicBlock(printFactorsFn, "PrintFactorsEntry")
builder.positionBuilderAtEnd(printFactorsEntry)
	-- Variables
	local value = builder.buildAlloca(int32Type, "Value")
	local divisor = builder.buildAlloca(int32Type, "Divisor")

	builder.buildStore(llvm.getParam(printFactorsFn, 0), value)
	builder.buildStore(llvm.constInt(int32Type, 2, false), divisor)

	local loop = llvm.appendBasicBlock(printFactorsFn, "Loop")
	local modulo = llvm.appendBasicBlock(printFactorsFn, "Modulo")
	local divide = llvm.appendBasicBlock(printFactorsFn, "Divide")
	local increment = llvm.appendBasicBlock(printFactorsFn, "Increment")
	local done = llvm.appendBasicBlock(printFactorsFn, "Done")

	builder.buildBr(loop)

	builder.positionBuilderAtEnd(loop)
		local valueDeref = builder.buildLoad(value, "ValueDeref")
		local divisorDeref = builder.buildLoad(divisor, "DivisorDeref")

		-- Test *divisor > *value and branch
		local compare = builder.buildICmp(llvm.intSGT, divisorDeref, valueDeref, "DivisorCompare")
		builder.buildCondBr(compare, done, modulo)

	builder.positionBuilderAtEnd(modulo)
		local valueDeref = builder.buildLoad(value, "ValueDeref")
		local divisorDeref = builder.buildLoad(divisor, "DivisorDeref")
		
		-- *value % *divisor
		local modResult = builder.buildSRem(valueDeref, divisorDeref, "ModResult")

		-- Test *value % *divisor == 0 and branch
		local dividesEvenly = builder.buildICmp(llvm.intEQ,
			modResult, llvm.constInt(int32Type, 0, false), "DividesEvenly")
		builder.buildCondBr(dividesEvenly, divide, increment)

	builder.positionBuilderAtEnd(divide)
		local valueDeref = builder.buildLoad(value, "ValueDeref")
		local divisorDeref = builder.buildLoad(divisor, "DivisorDeref")

		-- printf divisor
		local printfArgs = valueArray {
			builder.buildGlobalStringPtr("%d ", "tmpstring"),
			divisorDeref
		}
		builder.buildCall(printfFn, printfArgs, 2, "tmpcall");

		-- *value = *value / *divisor
		local divideResult = builder.buildExactSDiv(valueDeref, divisorDeref, "DivideResult")
		builder.buildStore(divideResult, value)

		-- Restart loop
		builder.buildBr(loop)

	builder.positionBuilderAtEnd(increment)
		local divisorDeref = builder.buildLoad(divisor, "DivisorDeref")

		-- *divisor = *divisor + 1
		local addResult = builder.buildAdd(divisorDeref,
			llvm.constInt(int32Type, 1, false), "AddResult")
		builder.buildStore(addResult, divisor)

		-- Restart loop
		builder.buildBr(loop)

	builder.positionBuilderAtEnd(done)
		builder.buildRetVoid()

backend:optimize(mainFn)
backend:optimize(printFactorsFn)

backend:debugDump()
backend:emitToFile()
