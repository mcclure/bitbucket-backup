-- Room and noise managers

LEFT  = 1
UP    = 2
RIGHT = 3
DOWN  = 4
STAR  = "STAR"
LOOP  = "LOOP"

function i(x) return x end

-- What is a room?
-- room[1] is a thing to print-- if any
-- room.enter and room.exit will be called if present
-- On exit, dir passed first to handle, then to goto table (key is dir, STAR is backup)
-- All rooms assumed to have goto or handle, if not, they won't be entered (enter called anyway)

class "Roomer" (Ent)

function Roomer:Roomer(spec)
	local s = {typer={print=i}, map={start={goto={}}}}
	Ent.Ent(self, tableMerge(s, spec))
	self.at = self.map[ self.start or "start" ]
end

function Roomer:printRoom(str)
	self.typer:print(str)
	self.typer:print("\n\n")
end

function Roomer:maybe(f)
	if f then f(self) end
end

function Roomer:step(dir)
	local room = self.at
	
	if fm.chatty then print(room) end -- Debug
	if not room then return end
	
	if room.handle then
		room.handle(self, dir)
	else
		local nextroom, nextkey
		nextroom = room.goto[dir] or room.goto[STAR] or (room.goto[LOOP] and room)
		if type(nextroom) == "string" then
			-- Little debug things
			if fm.chatty then print("--> " .. nextroom) end
			fm.lastAt = nextroom
			
			nextroom = self.map[nextroom]
		end
		
		if nextroom then
			self:maybe(room.exit)
			if nextroom[1] then self:printRoom(nextroom[1]) end
			self:maybe(nextroom.enter)
			if nextroom.handle or nextroom.goto then self.at = nextroom end
		elseif _DEBUG then
			self:printRoom("[DANGLING ROOM POINTER]")
		end
	end
end

class "InputRoomer" (Roomer)

function InputRoomer:InputRoomer(spec)
	Roomer.Roomer(self, spec)
end

function InputRoomer:onInput()
	for i,v in ipairs({KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_DOWN}) do
		if pressed[v] then self:step(i) end
	end
end

function chat() fm.chatty = true end
function reset(c) fm = c and {chatty=true} or nil end

function sanity_check_room(map, room)
	if room.goto then for k,v in pairs(room.goto) do
		if type(v) == "string" then
			local room = map[v]
			if not room then
				print("SANITY FAIL-- DEAD LINK: \"" .. v .. "\"")
			end
		else
			sanity_check_room(map, v)
		end
	end end
end
function sanity_check(map)
	for k,v in pairs(map) do sanity_check_room(map, v) end
end