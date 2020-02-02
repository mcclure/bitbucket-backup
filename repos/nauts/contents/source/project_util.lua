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
			return string.format("<object %s>", tbl.__info.__class.__info.__name)
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

function objs()
	a = {}
	s = bridge:room_objs()
	c = 0
	len = s:size()
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
	screen():setTransform(entity, Vector3(x,y,0), 0)
	screen():wakeUp(entity) -- So gravity works
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

-- I use this to check whether the target run-script phase in Xcode is acting up, ignore it
function test()
	print("BLARG7")
end
