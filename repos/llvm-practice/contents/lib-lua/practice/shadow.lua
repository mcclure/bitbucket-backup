require("lib-lua/practice/ast")

-- caller.lua helper: Just grab the caller's caller
local function return1() return 1 end
local function obviousCaller() return findCaller(return1) end
local function externalCallerSearch(t)
	if not endswith(t.filename, "/shadow.lua") then
		return 0
	end
	return nil
end
local function externalCaller() return findCaller(externalCallerSearch) end

-- Convert AstType to shadow type
local function shadowTypeMake(v)
	local shadowTypeMetatable = {__index=cacheAndReturn(function(type, key)
		if key == 'ptr' then
			return shadowTypeMake(AstType{ptr=type.__astObject})
		end
		return nil
	end)}

	local t = {__astObject=v}
	setmetatable(t, shadowTypeMetatable)
	return t
end

-- Given a Lua value, return an AstExpr
local function literal(v) return AstExpr{op='literal', value=v, location=externalCaller()} end

-- Given either a Lua value or shadow value, return an AstExpr
local function filterToAst(v)
	if astLiteralAllowed(v) then return literal(v) else return v.__astObject end
end

-- Go out of way to define shadowExprMetatable outside closure for efficiency
local function binop(op2)
	return function(l, r)
		return shadowExprMake( AstExpr{op='bin', op2=op2, value={filterToAst(l), filterToAst(r)}, location=obviousCaller()} )
	end
end
local shadowExprMetatable = {
	__add = binop('+'), __sub = binop('-'), __mul = binop('*'), __div = binop('/'),
	__mod = binop('%'),
	__index = function(v, key)
		return shadowExprMake( AstExpr{op='deref', value=v.__astObject, idx=filterToAst(key), location=obviousCaller()})
	end
}

-- Convert AstType to shadow expr
function shadowExprMake(v)
	local t = {__astObject=v}
	setmetatable(t, shadowExprMetatable)
	return t
end

local function shadowfilterToAst(v) return shadowExprMake(filterToAst(v)) end

-- Dispenser for shadow types, currently contains only int[number]s
shadowType = {
	fromAst = shadowTypeMake,
	idk = {},
	vararg = {} -- Arbitrary object for comparing by reference
}
setmetatable(shadowType, {__index=cacheAndReturn(function(_, k)
	local llvmType = nil

	-- Automatically look up int constructors in LLVM headers
	local _,_,intmatch = string.find(k, '^int(%d+)$') -- # of bits
	if intmatch then
		llvmType = llvm['int'..intmatch..'Type']()
	end

	if llvmType then
		return shadowTypeMake(AstType{literal=llvmType})
	end

	return nil
end)})

-- Create an AstFn given a return shadow type, a list of arg shadow types, and a construction function
function shadowFunction(ret, args, gen)
	local astArgs = {}
	local vararg = false
	for _,v in ipairs(args) do
		if shadowType.vararg == v then
			vararg = true
		elseif shadowType.idk == v then
			table.insert(astArgs, AstTypeBuiltin.idk)
		else
			table.insert(astArgs, v.__astObject)
		end
	end

	local fn = AstFn{ret=ret and (ret == shadowType.idk and AstTypeBuiltin.idk or ret.__astObject),
		args=astArgs, vararg=vararg, location=obviousCaller()}
	if gen then
		local function nextBlock() return AstBlock{exprs={}} end
		local block = nextBlock()
		fn.spec.body = AstBody{entry = block}

		local genArgs = {}
		for i=1,#args do
			table.insert(genArgs, shadowExprMake(AstExpr{op='getParam',idx=i-1, location=string.format("(function at %s)", i, fn.location)}))
		end

		local function makeFnInvoke(key)
			return function(...)
				local args = {...}
				for i=1,#args do
					args[i] = filterToAst(args[i])
				end
				return shadowExprMake(AstExpr{op='call', fn=key, args=args, location=externalCaller()})
			end
		end

		local function build(gen, genArgs)
			execWithEnv(gen, {
				_var = emptyWithMetatable({
					__index = function(_, key)
						return shadowExprMake(AstExpr{op='load', var=key, location=obviousCaller()})
					end,
					__newindex = function(_, key, value)
						table.insert(block.spec.exprs, AstExpr{op='store', var=key, value=filterToAst(value), location=obviousCaller()})
					end
				}),
				_fn = emptyWithMetatable({
					__index = function(_, key)
						return makeFnInvoke(key)
					end
				}),
				_get = {
					_literal = shadowfilterToAst,
					_eq = binop('=='), _ne = binop('!='),
					_lt = binop('<'), _gt = binop('>'), _lte = binop('<='), _gte = binop('>=')
				},
				_do = {
					_return = function(v)
						table.insert(block.spec.exprs, AstExpr{op='return', value=filterToAst(v)})
					end,
					_fn = emptyWithMetatable({
						__index = function(_, key)
							local fn = makeFnInvoke(key)
							return function(...)
								table.insert(block.spec.exprs, fn(...).__astObject)
							end
						end
					}),
					_is = function(exp, type)
						table.insert(block.spec.exprs, AstExpr{op='type',
							value=filterToAst(exp), type=filterToAst(type)})
					end,
					_if = function(cond, y, n)
						local branchExpr = AstExpr{op='branch', cond=filterToAst(cond), location=obviousCaller()}
						table.insert(block.spec.exprs, branchExpr)

						local afterBlock = nextBlock()
						local yBlock = nextBlock()

						block = yBlock
						build(y)
						block.spec.exit = afterBlock

						local nBlock = afterBlock
						if n then
							nBlock = nextBlock()
							block = nBlock
							build(n)
							block.spec.exit = afterBlock
						end

						branchExpr.spec.y = yBlock   branchExpr.spec.n = nBlock
						block = afterBlock
					end,
					_while = function(cond, loop)
						local branchBlock = nextBlock()
						local loopBlock = nextBlock()
						local afterBlock = nextBlock()

						block.spec.exit = branchBlock

						local branchExpr = AstExpr{op='branch', cond=filterToAst(cond), y=loopBlock, n=afterBlock}
						table.insert(branchBlock.spec.exprs, branchExpr)

						block = loopBlock
						build(loop)
						block.spec.exit = branchBlock

						block = afterBlock
					end
				}
			}, genArgs)
		end

		build(gen, genArgs)

		if not ret and block:unterminated() then
			table.insert(block.spec.exprs, AstExpr({op='return'}))
		end
	end
	return fn
end

-- Create an AstModule given a name and a construction function
function shadowModule(name, gen)
	local module = AstModule{name=name}
	if gen then
		local env = {
			_fn = emptyWithMetatable {
				__newindex = function(_, key, value)
					module.namedFns[key] = value
				end,
				__call = function(func, ...)
					return shadowFunction(...)
				end
			},
			_type = shadowType
		}
		execWithEnv(gen, env)
	end
	return module
end
