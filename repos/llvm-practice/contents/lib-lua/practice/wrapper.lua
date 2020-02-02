-- Creates "fake module" wrapper objects

-- Wrap an __index function in this and it will cache the results in the table,
-- such that your __index function will only be called once per unique index.
function cacheAndReturn(fn)
	return function(table, key)
		local value = fn(table, key)
		table[key] = value
		return value
	end
end

-- Pass this one of the metatables below
function withMetatable(table, metatable)
	setmetatable(table, metatable)
	return table
end
function emptyWithMetatable(metatable)
	return withMetatable({}, metatable)
end

-- Transforms table.someCall into LLVM.LLVMSomeCall
llvmMetatable = {
	__index = cacheAndReturn(function(_, key)
		return LLVM["LLVM" .. capitalize(key)]
	end)
}

-- Transforms table.key(input) into target.key(arg, input)
-- In other words, this wraps a table while prebinding all function calls with one argument 
function bind1Metatable(target, arg)
	return {
		__index = cacheAndReturn(function(table, key)
			local func = target[key]
			return function(...) return func(arg, ...) end
		end)
	}
end

-- Takes a function and a table, and executes fn with env as its _G.
-- Resets everything at completion, but does not use env's metatable during execution.
function execWithEnv(fn, env, args)
	local envOldMetatable = getmetatable(env)
	local fnOldEnv = getfenv(fn)

	-- Make fn's original env available as a parent scope
	local envMetatable = {__index=fnOldEnv}
	if (envOldMetatable) then setmetatable(env, nil) end -- Double-setting metatable not allowed
	setmetatable(env, envMetatable)

	-- Run fn
	setfenv(fn, env)
	fn(unpack(args or {}))

	-- Restore original state
	setfenv(fn, fnOldEnv)
	setmetatable(env, envOldMetatable)
end
