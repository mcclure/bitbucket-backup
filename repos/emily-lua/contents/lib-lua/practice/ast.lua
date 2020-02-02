local phases = {
	'register',
	'typecheck',
	'backendRegister',
	'backendEmit'
}
local phaseLookup = {} for i,v in ipairs(phases) do phaseLookup[v] = i end
local tab = "    "

-- caller.lua helper: Walk up the chain looking for _init calls
local function makeFindCallerSearch()
	local countdown = 1 -- Toss top frame which will be setLocation
	local function searcher(t)
		if countdown <= 0 then
			-- Stacks look like: _init, call_ctor, [gibberish], repeat
			-- The first time we see the cycle broken, we have found the true caller
			if t.functionname == "_init" and endswith(t.filename, "/ast.lua") then
				countdown = 3
			else
				return 0
			end
		end
		countdown = countdown - 1
		return nil
	end
	return searcher
end

-- TODO: Clarify which fields may be modified in an AST object after construction

-- Base class for all AST objects
-- Statics:
--     recurKey, string option: field name in module, used by recur mechanism
-- Vars:
--     spec: table, containing original args. Never modified (referenced items may be)
--     phaseReached: integer, highest key from phases phase() has been called with
--     llvmObj: ptr option, output of render()
class.AstBase()
	function AstBase:_init(spec)
		self.spec = spec
		self.phaseReached = 0
	end

	-- Some objects should track their creation point
	function AstBase:setLocation()
		self.location = self.spec.location or findCaller(makeFindCallerSearch())
		if findCallerDebug then print(self.location) end
	end

	-- Human readable description of object and its provenance
	function AstBase:desc()
		return self:descTag() .. (self.location and ", " .. self.location or "")
	end

	-- [TO OVERRIDE] Human readable description minus location
	function AstBase:descTag()
		return "unknown AST object"
	end

	-- Generate then return llvmObj
	function AstBase:llvm()
		if not self.llvmObj then
			self.llvmObj = self:render()
		end
		return self.llvmObj
	end

	-- [TO OVERRIDE] Create the LLVM object which corresponds to this specification
	function AstBase:render() -- Some objects do not support llvm(), others can only generate at certain times
		error("Could not render this object. You may need to call emit() first")
	end

	-- Temporarily register the self object with the backend and run a function
	-- This is used so code can tell what the "current" module, function, body etc are
	function AstBase:recur(fn)
		local label = self.recurKey
		if label then
			local prev = backend[label] -- Allow nesting
			backend[label] = self
			fn()
			backend[label] = prev
		else -- Some objects don't register with the module, but phase() still passes through here
			fn()
		end
	end

	-- Each compiler phase performs a DFS on the graph of AST objects.
	-- To visit a single object for a particular phase, call it with the phase name here.
	-- This function ensures the object hasn't been visited this phase, then calls the function of the given name.
	function AstBase:phase(name)
		local phaseNumber = phaseLookup[name]
		if self.phaseReached and self.phaseReached >= phaseNumber then return end
		self.phaseReached = phaseNumber

		self:recur(function()
			if self[name] then self[name](self) end

			self:descend(name)
		end)
	end

	-- [TO OVERRIDE] Here an object should call phase() on all AST objects it references.
	function AstBase:descend(name) end

-- Representation for a module (ie, an object file). Call emit() to create program
-- Spec Vars:
--     name, string option: name to save module to on emit (default 'main')
--     namedFns, table: map of name to AstFn
-- Vars:
--     name, string: see spec
--     namedFns, table: see spec
--     currentModule, currentFn, currentBody, currentBlock: objects registered by recur()
class.AstModule(AstBase)
	function AstModule:_init(spec)
		self:super(spec)
		self.namedFns = self.spec.namedFns or {}
		self.name = self.spec.name or "main"
	end

	AstModule.recurKey = 'currentModule'

	-- Fill out module in backend
	function AstModule:compile()
		if self.compiled then return end
		 -- Set a global for the duration of this function
		if backend then error("AstModule:compile called, but backend already exists") end
		backend = Backend(self.name)
		
		self:recur(function()
			-- Ensure functions know names
			for k,v in pairs(self.namedFns) do
				v.name = k
			end

			-- Run all phases
			for _,v in ipairs(phases) do
				self:descend(v)
			end

			-- Process
			backend:verify()
			for _,v in pairs(self.namedFns) do
				backend:optimize(v.llvmObj)
			end
		end)
		
		self.backend = backend
		backend = nil
		self.compiled = true
	end

	function AstModule:emit()
		if not self.compiled then self:compile() end

		-- Write
		self.backend:debugDump()
		self.backend:emitToFile()
	end

	-- Phases and overrides

	function AstModule:descTag()
		return 'module "' .. self.name .. '"'
	end

	function AstModule:descend(name)
		for _,v in pairs(self.namedFns) do
			v:phase(name)
		end
	end

	-- Lookup/sanitization for functions
	function AstModule:fn(name)
		-- Note it is assumed ALL modules are registered manually before emission starts
		if type(name) == 'string' then
			return self.namedFns[name]
		else
			return name -- Assume this is a function object already
		end
	end

	function AstModule:debugTypeSummary()
		local str = ""
		local any = false
		for _,v in pairs(self.namedFns) do
			if any then str = str .. "\n" end
			any = true
			str = str .. v:debugTypeSummary()
		end
		return str
	end

-- Representation for a function
-- Spec vars:
--     ret, (AstType or idk) option: return type (nil for bool)
--     arg, (AstType or idk) list option: types of arguments
--     vararg, bool: true if function is vararg
--     body, AstBody option: Function body
-- Vars:
--     llvmType, ptr [after backendRegister]: function-type object
--     name, string [after module assigns it]: name this function will be registered at
--     ret, AstTyped: Dummy "var" used to track type of return value
--     args, AstTyped list: Dummy "var" used to track types of arguments
class.AstFn(AstBase)
	function AstFn:_init(spec)
		self:super(spec)
		self:setLocation()
	end

	AstFn.recurKey = 'currentFn'

	-- Phases and overrides

	function AstFn:descTag()
		return "function " .. (self.name and '"' .. self.name .. '"' or "(unknown)")
	end

	function AstFn:descend(name)
		if self.spec.body then
			self.spec.body:phase(name)
		end
	end

	function AstFn:register()
		local function filterToDummy(i,x)
			spec = {fnIdx=i, fnParent=self}
			if not x then spec.type = AstTypeBuiltin.void
			elseif x ~= AstTypeBuiltin.idk then spec.type = x end
			return AstTypedParam(spec)
		end
		self.args = {}
		for i,v in ipairs(self.spec.args or {}) do table.insert(self.args, filterToDummy(i,v)) end
		self.ret = filterToDummy(-1, self.spec.ret)
	end

	function AstFn:backendRegister()
		self.ret:typeRequire() -- TODO: Fail earlier?
		for _,v in ipairs(self.args) do v:typeRequire() end

		self.llvmObj = backend.module.addFunction(self.name, self:type())
	end

	-- Fetch the LLVMFunctionType for this function
	function AstFn:type()
		if not self.llvmType then
			local args = {}
			for i,v in ipairs(self.args or {}) do
				table.insert(args, v:type():llvm())
			end
			self.llvmType = llvm.functionType(self.ret:type():llvm(),
				typeArray(args), #args, self.spec.vararg and true or false)
		end
		return self.llvmType
	end

	function AstFn:debugTypeSummary()
		local str = "function " .. self.name .. "; " .. takeMessage(llvm.printTypeToString(self:type()))
		if self.spec.body then str = str .. "\n" .. self.spec.body:debugTypeSummary() end
		return str
	end

-- Representation for a function body
-- Spec vars:
--     namedBlocks, table option: map of block name to AstBlock
--     entry, string or AstBlock: block to execute first
-- Vars:
--     namedBlocks, table: see spec
--     anonBlocks, AstBlock list: all blocks declared inline in tree
--     namedVars, table: map of var name to AstVar
--     anonVars, AstVar list: all vars declared inline in tree
--     entry, AstBlock [after register]: Block calculated from spec vars
--     llvmAllocaBlock, ptr [after backendRegister]: llvmObj for pre-entry block defining variables
class.AstBody(AstBase)
	function AstBody:_init(spec)
		self:super(spec)
		self.namedBlocks = self.spec.namedBlocks or {}
		self.anonBlocks = {}
		self.namedVars = {}
		self.anonVars = {}
	end

	AstBody.recurKey = 'currentBody'

	-- Phases and overrides

	function AstBody:descTag()
		return "unknown function body"
	end

	function AstBody:descend(name)
		if name == 'register' then return end -- This pass descends manually

		for _,v in pairs(self.namedBlocks) do
			v:phase(name)
		end
		for _,v in ipairs(self.anonBlocks) do
			v:phase(name)
		end
	end

	function AstBody:register()
		for k,v in pairs(self.namedBlocks) do
			v.name = k
			v:phase('register')
		end

		if not self.spec.entry then error("Block without entry") end
		self.entry = self:registerBlock(self.spec.entry)
	end

	function AstBody:backendRegister()
		self.llvmAllocaBlock = llvm.appendBasicBlock(backend.currentFn.llvmObj, "")
	end

	function AstBody:backendEmit()
		-- Instantiate vars
		backend.builder.positionBuilderAtEnd(self.llvmAllocaBlock)

		local function alloca(v)
			v:typeRequire()
			v.llvmObj = backend.builder.buildAlloca(v:type():llvm(), v.name or "anon")
		end

		for _,v in pairs(self.namedVars) do alloca(v) end
		for _,v in ipairs(self.anonVars) do alloca(v) end

		backend.builder.buildBr(self.entry.llvmObj)
	end

	-- Lookup/sanitization for a block during register phase. Accepts either a name or a block object.
	function AstBody:registerBlock(b)
		if type(b) == 'string' then return self.namedBlocks[b] end
		b:phase('register')
		return b
	end

	-- Lookup/sanitization for a var during register phase. Accepts either a name or a var object.
	function AstBody:registerVar(v, create)
		if type(v) == 'string' then
			if self.namedVars[v] then
				v = self.namedVars[v]
			else
				local name = v
				v = AstVar{}
				v.name = name
			end
		end
		if create then v.valid = true end
		v:phase('register')
		return v
	end

	function AstBody:debugTypeSummary()
		local str = ""
		local any = false
		local function plus(tag, var)
			if any then str = str .. "\n" end
			any = true
			str = str .. tab .. tag .. "; " .. takeMessage(llvm.printTypeToString(var:type():llvm()))
		end
		for k,v in pairs(self.namedVars) do plus("var " .. k, v) end
		for _,v in ipairs(self.anonVars) do plus("anonymous var", v) end
		return str
	end

-- Representation for a block within a function
-- Spec vars:
--     exprs, AstExpr list option: block does this in this order
--     exit, AstBlock or string: block to execute after
-- Vars:
--     exprs, AstExpr list: see spec
--     exit, AstBlock: Block calculated from spec vars
class.AstBlock(AstBase)
	function AstBlock:_init(spec)
		self:super(spec)
		self.spec.exprs = self.spec.exprs or {}
	end

	AstBlock.recurKey = 'currentBlock'

	-- Phases and overrides

	function AstBlock:descTag()
		return "unknown basic block"
	end

	function AstBlock:descend(name)
		for _,v in ipairs(self.spec.exprs) do
			v:phase(name)
		end
	end

	function AstBlock:register()
		if not self.name then table.insert(backend.currentBody.anonBlocks, self) end

		if self.spec.exit and self:unterminated() then
			self.exit = backend.currentBody:registerBlock(self.spec.exit)
		end
	end

	function AstBlock:backendRegister()
		self.llvmObj = llvm.appendBasicBlock(backend.currentFn.llvmObj, self.name or "anon")
	end

	function AstBlock:backendEmit()
		backend.builder.positionBuilderAtEnd(self.llvmObj)
		for i,v in ipairs(self.spec.exprs) do
			v:llvm()
		end
		if self.exit then
			backend.builder.buildBr(self.exit.llvmObj)
		end
	end

	-- Returns true if block is unterminated
	function AstBlock:unterminated()
		local exprCount = #self.spec.exprs
		if exprCount < 1 then return true end
		local finalOp = self.spec.exprs[exprCount].spec.op
		return not (finalOp == 'branch' or finalOp == 'return')
	end

-- Representation for a variable type
-- Spec vars:
--     literal, ptr option: The LLVM value is known and it is this
--     ptr, AstType option: This is a pointer to another type
class.AstType(AstBase)
	function AstType:_init(spec) self:super(spec) end

	-- Phases and overrides

	function AstType:descTag()
		return "unknown type"
	end

	function AstType:render()
		if self.spec.literal then return self.spec.literal end
		if self.spec.ptr then
			return llvm.pointerType(self.spec.ptr:llvm(), 0)
		end
		return llvm.voidType()
	end

	-- Throw error if other type is incompatible with this one
	function AstType:compatible(type)
		return self:llvm() == type:llvm()
	end

	function AstType:canDeref()
		return toboolean(self.spec.ptr)
	end

	function AstType:deref()
		return self.spec.ptr
	end

AstTypeBuiltin = {
	idk = {}, -- to compare to
	int32 = AstType{literal=llvm.int32Type()},
	bool = AstType{literal=llvm.int1Type()},
	string = AstType{ptr=AstType{literal=llvm.int8Type()}},
	void = AstType{}
}

-- Representation for something (expr or var) which can be typed
-- Spec vars:
--    type, AstType option: Type is known
-- Vars:
--    typeValue, AstType option: Type is known (potentially from spec)
--    typeReason, AstExpr option: If typeValue, expr to "blame"
--    typeParent, AstTyped option: Type decisions should be delegated here
-- TODO: typeConstraint (pointer-to, not-void)
class.AstTyped(AstBase)
	function AstTyped:_init(spec)
		self:super(spec)
		if self.spec.type then self.typeValue = self.spec.type end
		self.typeConstraint = self.spec.typeConstraint or {}
	end

	AstTyped.Constraint = class()
		function AstTyped.Constraint:_init(spec) self.spec = spec end

	-- Phases and overrides

	function AstTyped:descTag()
		return "type variable"
	end

	-- What AstTyped is ultimately responsible for type decisions here? Can be self
	function AstTyped:typeParentRoot()
		if self.typeParent then return self.typeParent:typeParentRoot()
		else return self end
	end

	-- Current calculated type
	function AstTyped:type()
		return self:typeParentRoot():typeResolve().typeValue
	end

	-- Unify with type of other AstTyped
	-- Currently, argument is assumed "older"
	function AstTyped:typeUnify(other)
		local selfRoot = self:typeParentRoot():typeResolve()
		local otherRoot = other:typeParentRoot():typeResolve()
		if selfRoot.typeValue and otherRoot.typeValue then
			if not selfRoot.typeValue:compatible(otherRoot.typeValue) then
				self:typeError(selfRoot.typeValue, otherRoot.typeValue, otherRoot)
			end
		else
			-- Make one of these roots the root of the other.
			local newRoot = nil local newChild = nil
			if selfRoot.typeValue then
				newRoot = selfRoot
				newChild = otherRoot
			else
				newRoot = otherRoot
				newChild = selfRoot
			end

			-- FIXME: This is just not the right way to do this.
			newChild.typeParent = newRoot
			if newChild.typeConstraint.deref then
				if newRoot.typeConstraint.deref then
					newChild.typeConstraint.deref:typeUnify(newRoot.typeConstraint.deref)
				else
					if newRoot.typeValue then
						newChild:typeConstraintSatisfy(newRoot.typeValue)
					else
						newRoot.typeConstraint.deref = newChild.typeConstraint.deref
						newChild.typeConstraint.deref = nil
					end
				end
			end
		end
	end

	-- Absolute demand that this Typed MUST have this specific AstType
	function AstTyped:typeAssign(type, reason, alreadyResolved)
		local selfRoot = alreadyResolved and self or self:typeParentRoot():typeResolve()
		if selfRoot.typeValue then
			if not selfRoot.typeValue:compatible(type) then
				self:typeError(selfRoot.typeValue, type, reason)
			end
		else
			selfRoot.typeValue = type
			selfRoot.typeReason = reason
			selfRoot:typeConstraintSatisfy(type)
		end
	end

	-- Check constraints to see if they suddenly produce a type and return self
	function AstTyped:typeResolve()
		if not self.typeValue and self.typeConstraint then
			local deref = self.typeConstraint.deref
			if deref then
				local derefType = deref:type()
				if derefType then
					if derefType:canDeref() then
						self:typeAssign(derefType:deref(), deref, true)
					else
						error("Can't deref!")
					end
				end
			end
		end
		return self
	end

	function AstTyped:typeConstraintSatisfy(type)
		local deref = self.typeConstraint.deref
		if deref then
			self.typeConstraint.deref = nil
			deref:typeAssign(AstType{ptr=type}, self)
		end
	end

	-- Print human readable type error and fail
	function AstTyped:typeError(selfType, otherType, otherReason)
		printerr("Type unification failed.")

		local function printType(idx, type, reason)
			printerr("\nType #" .. idx .. " is:")
			llvm.dumpType(type:llvm())
			while reason do
				printerr("because of " .. reason:desc())
				reason = reason.typeReason
			end
		end

		printType(1, selfType, self)
		printType(2, otherType, otherReason)

		error()
	end

	-- Assert this Typed has a type
	function AstTyped:typeRequire()
		if not self:type() then error("Type could not be deduced for " .. self:desc()) end
	end

-- Representation for a parameter within a function, used only in type solving
-- Spec vars:
--    fnIdx, int: Which parameter is this? (-1 if this is a return value)
--    fnParent, AstFn: What function does this belong to?
class.AstTypedParam(AstTyped)
	function AstTypedParam:_init(spec) self:super(spec) end

	-- Phases and overrides

	function AstTypedParam:descTag()
		return (self.spec.fnIdx and self.spec.fnIdx < 0 and "return value" 
			or "parameter " .. (self.spec.fnIdx or "(INVALID INDEX)")) -- FIXME: This is 0-indexed. Is that right?
			.. " of " .. (self.spec.fnParent and self.spec.fnParent:descTag() or "(INVALID FUNCTION)")
	end

-- Representation for a variable within a function
-- Vars:
--    name, string option: assigned name
class.AstVar(AstTyped)
	function AstVar:_init(spec) self:super(spec) end

	-- Phases and overrides

	function AstVar:descTag()
		return "variable " .. (self.name and '"' .. self.name .. '"' or "(unknown)")
	end

	function AstVar:register()
		if self.name then
			if backend.currentBody.namedVars[self.name] then error("Duplicate AstVar for name "..self.var) end
			backend.currentBody.namedVars[self.name] = self
		else
			table.insert(backend.currentBody.anonVars, self)
		end
	end

-- Representation for an expression or statement within a function
-- Spec vars:
--     op, string: what kind of expr
--         Valid ops: literal, bin, return, store, load, getParam, deref, branch, call, type
--     op2, string option [if op=='bin']: what kind of binary operation
--     value, AstExpr list [if op=='bin'] or AstExpr: Operand or operands
--     var, string or AstVar [some ops only]: variable to operate on
--     idx, AstExpr [if op=='deref'] int [if op=='getParam']: key to look up in target
--     cond, AstExpr option [if op=='branch']: expr returns whether to branch to y or n
--     y, string or AstBlock [if op=='branch']: branch on cond success or (no cond specified)
--     n, (string or AstBlock) option [if op=='branch']: branch on cond failure
--     fn, string [if op=='fn']: name of function to call
--     args, string list [if op=='fn']: arguments for function call
-- Vars:
--     var, AstVar [after register] [some ops only]: variable to operate on
--     y, AstBlock [if op=='branch']: resolved AstBlock of spec y
--     n, AstBlock option [if op=='branch']: resolved AstBlock of spec n
class.AstExpr(AstTyped)
	function AstExpr:_init(spec)
		self:super(spec)
		self:setLocation()
	end

 	-- Utility: get table entry corresponding to self op
	function AstExpr:switch(table)
		local fn = table[self.spec.op]
		if fn then return fn() end
	end

	-- Overrides and phases

	function AstExpr:descTag()
		return self:switch {
			literal = function() return "literal" end,
			bin = function() return self.spec.op2 and self.spec.op2 .. " operation" or "(INVALID BINARY OPERATION)" end,
			['return'] = function() return "return instruction" end,
			store = function() return "store operation for " .. (self.var and self.var:descTag() or "(INVALID VARIABLE)") end,
			load = function() return "load operation for " .. (self.var and self.var:descTag() or "(INVALID VARIABLE)") end,
			getParam = function() return "load for parameter " .. (self.spec.idx and self.spec.idx+1 or "(INVALID INDEX)") end,
			deref = function() return "dereference operation" end,
			branch = function() return "branch instruction" end,
			call = function() return "function call " .. (self.spec.fn and '"' .. self.spec.fn .. '"' or "(INVALID FUNCTION)") end,
			type = function() return "type annotation" end
		}
	end

	function AstExpr:descend(name)
		local function hit(v)
			if v then v:phase(name) end
		end
		local function hitValue()
			hit(self.spec.value)
		end

		self:switch {
			bin = function()
				hit(self.spec.value[1])
				hit(self.spec.value[2])
			end,
			['return'] = function()
				hitValue()
			end,
			store = function()
				hit(self.var)
				hitValue()
			end,
			load = function()
				hit(self.var)
			end,
			deref = function()
				hitValue()
				hit(self.spec.idx)
			end,
			branch = function()
				hit(self.spec.cond)
				for _,k in ipairs({"y", "n"}) do hit(self.spec[k]) end
			end,
			call = function()
				for _,v in ipairs(self.spec.args) do hit(v) end
			end,
			type = function()
				hitValue()
				hit(self.var)
			end
		}
	end

	function AstExpr:register()
		self:switch {
			literal = function() -- Phase cheat: Types are known so I'd prefer them set before inference starts
				local literalTypeMap = {
					number = AstTypeBuiltin.int32,
					string = AstTypeBuiltin.string,
					boolean = AstTypeBuiltin.bool
				}
				local luaType = type(self.spec.value)
				self.typeValue = literalTypeMap[ luaType ]
				if not self.typeValue then
					error("Don't know how to translate this " ..luaType.. " into a literal: " ..tostring(self.spec.value))
				end
			end,
			bin = function() -- Phase cheat: see literal
				-- Once more numeric types exist than int32 this will no longer be possible
				local bool = AstTypeBuiltin.bool local int32 = AstTypeBuiltin.int32
				local outType = {
					['>'] = bool,  ['<'] = bool,  ['>='] = bool, ['<='] = bool,
					['=='] = bool, ['!='] = bool,
					['+'] = int32, ['-'] = int32, ['*'] = int32, ['/'] = int32,
					['%'] = int32,
				}
				self.typeValue = outType[self.spec.op2]
			end,
			store = function()
				self.var = backend.currentBody:registerVar(self.spec.var, true)
				-- Phase cheat: see literal
				self.typeValue = AstTypeBuiltin.void
			end,
			load = function()
				self.var = backend.currentBody:registerVar(self.spec.var)
			end,
			branch = function()
				for _,k in ipairs({"y", "n"}) do
					if self.spec[k] then
						self.spec[k] = backend.currentBody:registerBlock(self.spec[k])
					end
				end

				-- Phase cheat: see literal
				self.typeValue = AstTypeBuiltin.void
			end,
			type = function()
				if self.spec.var then
					self.var = backend.currentBody:registerVar(self.spec.var)
				end
			end
		}
	end

	function AstExpr:typecheck()
		-- Phase cheat: Check structural (non-type) issues here cuz it's convenient
		local badfmt = self:switch {
			bin = function() return not self.spec.value or #self.spec.value ~= 2 end,
			store = function() return not self.var or not self.spec.value end,
			load = function() return not self.var end,
			deref = function() return not self.spec.value end,
			branch = function() return not self.spec.y or (self.spec.cond and not self.spec.n) end,
			type = function() return not self.spec.type or not (self.spec.value or self.var) end
		}
		if badfmt then error("Ill-formed AstExpr with operation " .. self.spec.op) end

		-- Infer
		self:switch {
			bin = function()
				local inType = AstTypeBuiltin.int32 -- Sorry that's all I can generate yet
				for _,v in ipairs(self.spec.value) do
					v:typeAssign(inType, self)
				end
			end,
			['return'] = function()
				if self.spec.value then
					self.spec.value:typeUnify(backend.currentFn.ret)
				else
					backend.currentFn.ret:typeAssign(AstTypeBuiltin.void, self)
				end
			end,
			store = function()
				self.var:typeUnify(self.spec.value)
			end,
			load = function()
				self:typeUnify(self.var)
			end,
			getParam = function()
				if self.spec.idx >= #backend.currentFn.args then
					error("This function does not have an argument " .. self.spec.idx)
				end
				self:typeUnify(backend.currentFn.args[self.spec.idx+1])
			end,
			deref = function()
				self.spec.idx:typeAssign(AstTypeBuiltin.int32, self)
				self:typeUnify( AstTyped{typeConstraint={deref=self.spec.value}} )
			end,
			branch = function()
				self.spec.cond:typeAssign(AstTypeBuiltin.bool, self)
			end,
			call = function()
				local fn = backend.currentModule:fn(self.spec.fn)
				self:typeUnify(fn.ret)
				local selfArgCount = #self.spec.args
				local fnArgCount = #fn.args
				if selfArgCount ~= fnArgCount and not (fn.spec.vararg and selfArgCount > fnArgCount) then
					error("Wrong number of arguments")
				end
				for i=1,fnArgCount do
					self.spec.args[i]:typeUnify(fn.args[i])
				end
			end,
			type = function()
				local target = self.spec.value or self.var
				target:typeAssign(self.spec.type, self)
			end
		}
	end
	
	function AstExpr:render()
		return self:switch { -- Lookup table on op
			literal = function()
				local function literal(v)
					local t = type(v)
					if t == 'number' then -- Accept unwrapped numbers and strings
						return llvm.constInt(llvm.int32Type(), v, false)
					elseif t == 'string' then
						return backend.builder.buildGlobalStringPtr(v, "")
					elseif t == 'boolean' then
						return llvm.constInt(llvm.int1Type(), v and 1 or 0, false)
					else     -- Otherwise assume we've been passed an LLVM object
						return v
					end
				end

				return literal(self.spec.value)
			end,
			bin = function() -- Binary expression
				local a = self.spec.value[1]:llvm()
				local b = self.spec.value[2]:llvm()
				local function build(kind) return function()
					return backend.builder['build'..capitalize(kind)](a, b, "")
				end end
				local function iCmp(kind) return function()
					return backend.builder.buildICmp(llvm[kind], a, b, "")
				end end
				local switch2 = {
					['>'] = iCmp('intSGT'),  ['<'] = iCmp('intSLT'),
					['>='] = iCmp('intSGE'), ['<='] = iCmp('intSLE'),
					['=='] = iCmp('intEQ'),  ['!='] = iCmp('intNE'),
					['+'] = build('add'),    ['-'] = build('sub'),
					['*'] = build('mul'),    ['/'] = build('sDiv'),
					['%'] = build('sRem')
				}
				return switch2[self.spec.op2]()
			end,
			['return'] = function()
				if self.spec.value then
					local value = self.spec.value:llvm()
					backend.builder.buildRet(value)
				else
					return backend.builder.buildRetVoid()
				end
			end,
			store = function()
				return backend.builder.buildStore( self.spec.value:llvm(), self.var.llvmObj )
			end,
			load = function()
				return backend.builder.buildLoad( self.var.llvmObj, "" )
			end,
			getParam = function()
				return llvm.getParam(backend.currentFn.llvmObj, self.spec.idx)
			end,
			deref = function() -- TODO: Allow idx to be a list
				local ptr = self.spec.value:llvm()
				if self.spec.idx then
					ptr = backend.builder.buildGEP(ptr, valueArray {self.spec.idx:llvm()}, 1, "")
				end
				return backend.builder.buildLoad(ptr, "")
			end,
			branch = function()
				if self.spec.cond then
					return backend.builder.buildCondBr(self.spec.cond:llvm(), self.spec.y.llvmObj, self.spec.n.llvmObj)
				else
					return backend.builder.buildBr(self.spec.y.llvmObj)
				end
			end,
			call = function()
				if not self.spec.fn then error("Function not found "..self.spec.fn) end
				local llvmArgs = {}                    -- map :llvm() on self.spec.args
				for _,v in ipairs(self.spec.args) do
					table.insert(llvmArgs, v:llvm())
				end
				local fn = backend.currentModule:fn(self.spec.fn)
				return backend.builder.buildCall(fn.llvmObj, valueArray(llvmArgs), #llvmArgs, "")
			end,
			type = function()
				return nil -- Should code be executed?
			end
		}
	end

-- Return true if this variable is okay to use in AstExpr{op='literal'}
function astLiteralAllowed(v)
	local t = type(v)
	return t == 'number' or t == 'string' or t == 'cdata' or t == 'boolean'
end
