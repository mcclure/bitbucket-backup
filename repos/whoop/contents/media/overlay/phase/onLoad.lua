-- On Load

gm = { w = {} }
km = { on = 2, aon = 1, fdens = 5, flast=10 }

killDos()
dos = type_automaton()
dos:insert()
dosKm()

class "Walker"

function Walker:Walker(x,y,dx,dy, bx1,by1,bx2,by2, istep)
	if istep then self.tone = newTone(istep) end
	self.x = x self.y = y
	self.dx = dx or 0 self.dy = dy or 0
	self.p = 1
	self.istep = istep
	self.lat = 0
	
	self.bx1 = bx1 self.by1 = by1 self.bx2 = bx2 self.by2 = by2
end

function Walker:silentclone()
	return Walker(self.bx1,self.by1,1,0,self.bx1,self.by1,self.bx2,self.by2)
end

function Walker:insert()
	self.tone:Play(true)
	table.insert(gm.w, self)
end

function Walker:flip(at)
	local flipper = self:silentclone()
	for i=1,1000 do
		if i ~= 1 and flipper.x == self.bx1 and flipper.y == self.by1 then break end
		local offs = (i+at-1)%km.fdens
		if offs == 0 then 
			local s = dos:get(flipper.x,flipper.y)
			dos:set(flipper.x, flipper.y, string.char( (string.byte(s)+128)%256 ))
		end
		flipper:step()
	end
end

function Walker:step()
	self.x = self.x + self.dx
	self.y = self.y + self.dy
	self:correct()
end

function Walker:tick(refresh)
	if not self.mute and ticks % km.on == 0 then
		if self.flast then self:flip(self.flast) self.flast = nil end
		
		local invert = self.dy~=0
		dos:set(self.x,self.y,string.char(math.random(0,127)),invert)
		self:step()
		
		if self.left and self.left > 0 then
			self.flast = self.lat
			self:flip(self.lat)
			self.lat = self.lat + self.ldir
			self.left = self.left - 1
		end
	end
	
	if ticks % km.aon == 0 then
		self.tone:setPitch(self.p)
	end
end

function Walker:clr(invert)
	dos:fill(self.bx1,self.by1,(self.bx2-self.bx1+1),1," ",invert)
	dos:fill(self.bx1,self.by1,1,(self.by2-self.by1+1)," ",invert)
	dos:fill(self.bx1,self.by2,(self.bx2-self.bx1+1),1," ",invert)
	dos:fill(self.bx2,self.by1,1,(self.by2-self.by1+1)," ",invert)
end

function Walker:turn()
	local temp = self.dx self.dx = self.dy self.dy = temp
	if self.dx ~= 0 then self.dx = -self.dx end
	
	if self.dx ~= 0 then self.p = self.p * 2
	else self.p = self.p / 2 end
	
	if self.tone then 
		local invert = self.dy~=0
		self:clr(invert)
	end
end

function Walker:correct()
	local did = false
	for i=1,4 do
		did = false
		if self.dx > 0 and self.x == self.bx2 then self:turn() did = true end
		if self.dy > 0 and self.y == self.by2 then self:turn() did = true end
		if self.dx < 0 and self.x == self.bx1 then self:turn() did = true end
		if self.dy < 0 and self.y == self.by1 then self:turn() did = true end
		if not did then break end
	end
end

km.remap = {
	[KEY_1] = function(i) return (11-i)*6-6 end,
	[KEY_3] = function(i) return (   i)*6-6 end,
	[KEY_2] = function(i) return (11-i)*6-3 end,
	[KEY_4] = function(i) return (11-i)*3-3 end,
	[KEY_5] = function(i) return 0 end,
	[KEY_6] = function(i) return 12 end,
	[KEY_7] = function(i) return 48 end,
	[KEY_8] = function(i) i = 11-i local k = {0,2,5,7,9} local r = k[(i % #k) + 1]+12*math.floor(i/#k) return r end,
	[KEY_9] = function(i) i = 11-i local k = {0, 4, 7} local r = k[(i % #k) + 1]+12*math.floor(i/#k) return r end,
	[KEY_0] = function(i) local k = {0, 4, 7, 9} local r = k[(i % #k) + 1]+12*math.floor(i/#k) return r end,
}

local normalmap = km.remap[KEY_1]

for i=0,11 do
	Walker(i,i,1,0, i,i,39-i,23-i, normalmap(i)):insert()
end

-- Set up input handler

down = {} pressed = {}
listeners = {} -- To get unicode-level key input
mouseDownAt = nil
mouseAt = nil

class "Input" (EventHandler)
function Input:Input()
	EventHandler.EventHandler(self)
end

function Input:handleEvent(e)
	local inputEvent = InputEvent(e)
	local key = inputEvent:keyCode()
	if e:getEventCode() == InputEvent.EVENT_MOUSEMOVE then
		delete(mouseAt) mouseAt = inputEvent:getMousePosition()
	elseif e:getEventCode() == InputEvent.EVENT_MOUSEDOWN then
		delete(mouseDownAt) mouseDownAt = inputEvent:getMousePosition()
		delete(mouseAt) mouseAt = vDup(mouseDownAt)
		down[inputEvent.mouseButton] = true
		pressed[inputEvent.mouseButton] = true
	elseif e:getEventCode() == InputEvent.EVENT_MOUSEUP then
		down[inputEvent.mouseButton] = nil
	elseif e:getEventCode() == InputEvent.EVENT_KEYDOWN then
		down[key] = true
		pressed[key] = true
		for k,v in pairs(listeners) do
			v:push({key, bridge:charCode(inputEvent)})
		end
	elseif e:getEventCode() == InputEvent.EVENT_KEYUP then
		down[key] = false
	end
end

do
	input = Input()

	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEMOVE)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEDOWN)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEUP)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYUP)
end