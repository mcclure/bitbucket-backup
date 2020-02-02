-- Namespace support for Lua. By Andi McClure. Version 1.1
--
-- Suggested usage: At the top of your entry point file, say
--     namespace = require "namespace"
-- In any file thereafter, say
--     namespace "somenamespace"
-- And globals will be shared between only those files using the same key.
--
-- See comments on functions for more esoteric usages.

local namespace_mt = {}
local namespace = {}
namespace_mt.__index = namespace
local namespace_return = setmetatable({}, namespace_mt)
namespace.spaces = {} -- Named namespaces
local origin
local preload = {}

local function table_clone(t) local out = {} for k,v in pairs(t) do out[k] = v end return out end

-- Get the table that a particular named namespace is stored in.
-- If the namespace doesn't exist, one will be created and registered.
-- Pass nil for name to get a unique single-use namespace.
-- Pass a table or string (namespace name) for "inherit" to set the default values when creating a new namespace.
function namespace.space(name, inherit)
	local space = name and namespace.spaces[name]
	if not space then
		-- Handle prepare()
		local preload_inherit, preload_construct
		if preload[name] then
			preload_inherit, preload_construct = unpack(preload[name])
			preload[name] = nil
			if preload_inherit then 
				if inherit and inherit ~= preload_inherit then
					error(string.format("Namespace \"%s\" given conflicting inherit instructions: \"%s\" in prepare() and \"%s\" later", name, preload_inherit, inherit))
				end
				inherit = preload_inherit
			end
		end

		-- "inherit" could be one of several types right now. change it to a table.
		if type(inherit) == "string" then
			local inherit_table = namespace.spaces[inherit]
			if not inherit_table then
				if preload[inherit] then -- Handle prepare()
					inherit_table = namespace.space(inherit)
				else
					error(string.format("Namespace \"%s\" tried to inherit from namespace \"%s\", but that doesn't exist", name, inherit))
				end
			end
			inherit = inherit_table
		elseif not inherit then -- No inherit specified, use default
			inherit = origin or getfenv(0)
		end
		-- Now that we have an inherit table, inherit from it
		space = table_clone(inherit)
		local inherit_mt = getmetatable(inherit)
		if inherit_mt then
			local space_mt = table_clone(inherit_mt)
			local inherit_mt_mt = getmetatable(inherit_mt)
			if inherit_mt_mt then setmetatable(space_mt, inherit_mt_mt) end
			setmetatable(space, space_mt)
		end
		-- Set up space
		space.namespace = namespace_return
		space.current_namespace = name
		if name then namespace.spaces[name] = space end
		if preload_construct then preload_construct(space) end -- Must do after "spaces" populated
	end
	return space
end

-- Set the globals of the calling file to the namespace named "name".
-- As with space(), "name" can be nil and "inherit" can be a default.
-- Can be called with "namespace()"
local function enter(_, name, inherit)
	setfenv(2, namespace.space(name, inherit))
end
namespace_mt.__call = enter

-- Fancy features

-- Create a lazy-load namespace.
-- On first attempt to load namespace 'inherit' will be inherited,
-- and 'construct' will be called with the new space table as argument.
function namespace.prepare(name, inherit, construct)
	if preload[name] then error(string.format("Called prepare() twice on namespace \"%s\"", name)) end
	if namespace.spaces[name] then error(string.format("Called prepare() on already-existing namespace \"%s\"", name)) end
	preload[name] = {inherit, construct}
end

-- Call this before calling require() or unpollute().
-- This freezes a "clean" globals table as the default table to inherit from when creating a namespace.
-- Pass no argument to use the "default environment" (the globals table registered to use for new requires)
-- Pass a table to set that table as the origin.
-- Pass a "namespace" argument to use that namespace as the origin.
function namespace.origin(t, inherit)
	origin = nil
	origin = type(t) == "table" and t or namespace.space(t, inherit)
end

-- If you believe a require has been leaving junk in the global namespace,
-- this will reset the "default environment" (the globals table registered to use for new requires)
-- to the origin. (The globals of the file calling unpollute() will be unaffected.)
-- Pass a "namespace" argument to use that namespace instead of a clean table. 
function namespace.unpollute(name, inherit)
	if not (origin or name or inherit) then
		error("Called unpollute() without setting an origin first")
	end
	setfenv(0, namespace.space(name, inherit))
end

-- This will perform a require, with the given namespace preset for globals.
-- If no name is given, it will make a new clean namespace (preventing it from polluting any globals table)
function namespace.require(torequire, name, inherit)
	local old_zero = getfenv(0)
	setfenv(0, namespace.space(name, inherit))
	local result = require(torequire)
	setfenv(0, old_zero)
	return result
end

-- Store stuff in here that you want to pass between namespaces
namespace.globals = {}

return namespace_return
