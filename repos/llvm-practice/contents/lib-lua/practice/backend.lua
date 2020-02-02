-- Main LLVM wrapper class

llvm = emptyWithMetatable(llvmMetatable)

class.Backend()
function Backend:_init(moduleName)
	self.moduleName = moduleName
	self.llvm = {
	    module = llvm.moduleCreateWithName(moduleName),
        builder = llvm.createBuilder()
    }

    local enginePtr = ffi.new("LLVMExecutionEngineRef[1]")
    if LLVM.LLVMCreateExecutionEngineForModule(enginePtr, self.llvm.module, errPtr) == 1 then
    	llvmErr("LLVMCreateExecutionEngineForModule")
    end
    self.llvm.engine = enginePtr[0]

    self.llvm.passManager = llvm.createFunctionPassManagerForModule(self.llvm.module)

    local function wrapperObject(o) return emptyWithMetatable(bind1Metatable(llvm, o)) end
    self.module = wrapperObject(self.llvm.module)
    self.builder = wrapperObject(self.llvm.builder)
	self.engine = wrapperObject(self.llvm.engine)
	self.passManager = wrapperObject(self.llvm.passManager)

    llvm.addTargetData(self.engine.getExecutionEngineTargetData(), self.llvm.passManager)
    self.passManager.addPromoteMemoryToRegisterPass()
    self.passManager.addInstructionCombiningPass()
    self.passManager.addReassociatePass()
    self.passManager.addGVNPass()
    self.passManager.addCFGSimplificationPass()
    self.passManager.initializeFunctionPassManager()
end

function Backend:dispose()
	self.pass_manager.disposePassManager()
    self.builder.disposeBuilder()
    self.module.disposeModule()
end

function Backend:debugDump()
	self.module.dumpModule()
end

function Backend:optimize(fn)
	self.passManager.runFunctionPassManager(fn)
end

function Backend:verify()
	llvm.verifyModule(self.llvm.module, llvm.abortProcessAction, nil)
end

function Backend:emitToFile()
	local targetRefPtr = ffi.new("LLVMTargetRef [1]")
	compileTriple = config.compileTriple or llvm.getDefaultTargetTriple()
	compileCpu = config.compileCpu or ""
	compileFeatures = config.compileFeatures or ""

	if llvm.getTargetFromTriple (compileTriple, targetRefPtr, errPtr) == 1 then
		llvmErr("LLVMGetTargetFromTriple")
	end
	if llvm.targetMachineEmitToFile(
			llvm.createTargetMachine(targetRefPtr[0], compileTriple, compileCpu, compileFeatures,
				llvm.codeGenLevelAggressive, llvm.relocDefault, llvm.codeModelSmall),
		self.llvm.module, cString(self.moduleName .. ".o"), llvm.objectFile, errPtr) == 1 then
		llvmErr("LLVMTargetMachineEmitToFile")
	end
end

function Backend:astModule(name, fn)
	
end
