-- Poly-friendly to_string function, adapted from http://lua-users.org/wiki/TableSerialization
function to_string( tbl, indent, done )
	indent = indent or 0
	done = done or {}
	if "nil" == type( tbl ) then
		return tostring(nil)
	elseif "table" == type( tbl ) then
		if done[tbl] then -- Protect against recursive structures
			return tostring(tbl)
		elseif tbl.__info and tbl.__info.__isclass then
			return string.format("<class %s>", tbl.__info.__name)
		elseif tbl.__info and tbl.__info.__class then
			local name = tbl.__info.__class.__info.__name
			if name == "Vector3" then
				return string.format("<vector %s, %s, %s>", tbl.x, tbl.y, tbl.z)
			elseif name == "Vector2" then
				return string.format("<vector %s, %s>", tbl.x, tbl.y)
			elseif name == "Color" then
				return string.format("<Color %s, %s, %s, %s>", tbl.r, tbl.g, tbl.b, tbl.a)
			else
				return string.format("<object %s>", tbl.__info.__class.__info.__name)
			end
		else 
			local sb = {}
			local first = true
			done[tbl] = true
			for key, value in pairs (tbl) do
				if first then first = false else table.insert(sb, ", ") end
				if "number" == type(key) then
					table.insert(sb, to_string(value, indent+1, done))
				else
					table.insert(sb, string.format("%s = %s",
							to_string(key, indent+1, done), to_string(value, indent+1, done)))
				end
			end
			return "{" .. table.concat(sb) .. "}"
		end
	elseif "string" == type( tbl ) then
		if indent>0 then -- Top-level strings don't get quoted
			return string.format("\"%s\"", tbl)
		else
			return tbl
		end
	else
		return tostring(tbl)
	end
end

-- Insert a custom print in place of the defaults.lua print, which calls to_string first
raw_print = _G["print"]
function better_print(str)
	raw_print(to_string(str))
end
_G["print"] = better_print

-- Wrappers
bridge = project_bridge()
function screen()
	return bridge:room_screen()
end

function scene()
	return bridge:room_scene()
end

function objs()
	local a = {}
	local s = bridge:room_objs()
	local c = 0
	local len = s:size()
	while c < len do
		table.insert(a, s:get(c))
		c = c + 1
	end
	return a
end

function id(idx)
	return bridge:room_id(idx)
end

function clear()
	bridge:clear()
end

function setPosition(entity, x, y)
	local physics = PhysicsScreen(screen())
	physics:setTransform(entity, Vector3(x,y,0), 0)
	physics:wakeUp(entity) -- So gravity works
end

function destroy_standalone_screen(s)
	bridge:room_remove_standalone_screen_all(s)
	delete(s)
end

function help(path)
	if "table" == type( path ) and path.__info and path.__info.__isclass then
		help(path.__info.__name)
	elseif "table" == type( path ) and path.__info and path.__info.__class then
		help(path.__info.__class.__info.__name)
	else
		print(bridge:help(path or ""))
	end
end

-- Memory management -- Polycode creates some memory management issues lua doesn't normally have.

function release_all(from)
	for i,one in ipairs(from) do
		delete(one)
	end
end

function memory_drain() -- Call at end of each onUpdate
	if autorelease then
		local to_release = autorelease
		autorelease = {}
		release_all(to_release)
	end
end

function memory_teardown() -- Call at end of each onClose
	memory_drain()
	autorelease = nil
	if retain then
		local to_release = retain
		retain = nil
		release_all(to_release)
	end
end

function memory_setup() -- Call at beginning of each onLoad
	memory_teardown() -- Just in case-- probably a noop.
	autorelease = {}
	retain = {}
end

function a(v) -- "Autorelease": delete at end of current frame
	table.insert(autorelease, v)
	return v
end
function r(v) -- "Retain": delete on room destruction
	table.insert(retain, v)
	return v
end
function i(v) -- "Identity": do nothing
	return v
end

-- Utility classes / methods

function vDup(v) -- Duplicate vector marked for destruction
	return Vector3(v.x,v.y,v.z or 0)
end
function vAdd(v,b) -- Since operator+ isn't exported
	return a(Vector3(v.x+b.x,v.y+b.y,(v.z or 0)+(b.z or 0)))
end
function vSub(v,b) -- Since operator- isn't exported
	return a(Vector3(v.x-b.x,v.y-b.y,(v.z or 0)-(b.z or 0)))
end
function vMult(v, f)
	return a(Vector3(v.x*f, v.y*f, (v.z or 0)*f))
end
function vNorm(v)
	local z = v.z or 0
	return math.sqrt(v.x*v.x + v.y*v.y + z*z)
end
function vNormalize(v,_extent)
	local extent = _extent or 1
	local norm = vNorm(v)
	if norm == 0 then return a(Vector3(0,0,0)) end
	return vMult(v,(1/norm)*extent)
end
function vSet(v,b)
	v.x = b.x v.y = b.y v.z = b.z or 0
end
function vSetPosition(e,v) -- Since setPosition(vector) isn't exported
	e:setPosition(v.x,v.y,v.z or 0)
end

function tableCount(e)
	local count = 0
	for a,b in pairs(e) do
		count = count + 1
	end
	return count
end

function tableCopy(t)
	local t2 = {}
	for k,v in pairs(t) do
		t2[k] = v
	end
	return t2
end

function tableDeepCopy(t)
	local t2 = {}
	for k,v in pairs(t) do
		if "table" == type( k ) then k = tableDeepCopy(k) end
		if "table" == type( v ) then v = tableDeepCopy(v) end
		t2[k] = v
	end
	return t2
end

function resetPtrLookup() -- Force GC of ptr wrapper objects
	for k,v in pairs(__ptr_lookup) do
		__ptr_lookup[k] = {}
	end
end

function pull(dst, src)
	if dst and src then
		for k,v in pairs(src) do
			dst[k] = v
		end
	end
end

function clamp(lo, v, hi)
	if v < lo then return lo end
	if v > hi then return hi end
	return v
end

function all(ary) -- For use with StringArray, EntityArray etc.
	if not ary then return nil end
	t = {}
	for i = 1,ary:size() do
		table.insert(t, ary:get(i-1))
	end
	return t
end

function string_empty(s)
	return not (s and #s>0)
end

function reboot() -- Or as near as possible anyway.
	bridge:load_room_txt("media/init.txt")
end

class "Queue" -- Queue w/operations push, pop, peek
function Queue:Queue()
	self.low = 1 self.count = 0
end
function Queue:push(x)
	self[self.low + self.count] = x
	self.count = self.count + 1
end
function Queue:pop()
	if self.count == 0 then
		return nil
	end
	local move = self[self.low]
	self[self.low] = nil
	self.count = self.count - 1
	self.low = self.low + 1
	return move
end
function Queue:peek()
	if self.count == 0 then
		return nil
	end
	return self[self.low]
end
function Queue:empty()
	return self.count == 0
end

-- Shader management

class "ShaderWrapper"

function ShaderWrapper:ShaderWrapper(parent, bindingnum, name, default)
	if not default then default = 0 end
	if not bindingnum then bindingnum = 0 end
	local b = parent:getScreenShaderMaterial():getShaderBinding(bindingnum)
	if "table" == type(default) then
		b:addLocalParamVector3(name, default);
	else
		b:addLocalParamNumber(name, default);
	end
	self.paramObject = b:getLocalParamByName(name)
	self:set(default)
end

function ShaderWrapper:set(value)
	if "table" == type(value) then
		self.paramObject:setVector3(value)
	else
		self.paramObject:setNumber(value)
	end
	self.last = value
end

function ShaderWrapper:get()
	return self.last
end

-- 'parent' is screen() or scene():displayScene():getDefaultCamera()
-- 'nametable' is a map of binding names to binding defaults
-- 'bindingnum' is a getShaderBinding() argument, can/should be nil
function shaderBindings(parent, nametable, bindingnum)
	local result = {}
	for name,default in pairs(nametable) do
		result[name] = ShaderWrapper(parent, bindingnum, name, default)
	end
	return result
end

-- I use this to check whether the target run-script phase in Xcode is acting up, ignore it
function test()
	print("BLARG4")
end
