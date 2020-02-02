-- On Load

gm = { w = {} }
km = { on = 2, aon = 1 }

killDos()
dos = type_automaton()
dos:insert()

class "Walker"

function Walker:Walker(x,y,dx,dy)
	self.tone = newTone()
	self.x = x self.y = y
	self.dx = dx or 0 self.dy = dy or 0
	self.p = 1 self.dp = 0 self.ddp = 0
end

function Walker:insert()
	self.tone:Play(true)
	table.insert(gm.w, self)
end

function Walker:tick(refresh)
	if refresh then
		dos:set(self.x,self.y,string.char(math.random(0,127)))
		self.x = self.x + self.dx
		self.y = self.y + self.dy
		self:correct()
	end
	
	if ticks % km.aon == 0 then
		self.p = self.p + self.dp
		local ddp = 0.9*(1 - self.p)
		self.dp = self.dp + ddp
		self.tone:setPitch(self.p)
	end
end

function Walker:turn()
	local temp = self.dx self.dx = self.dy self.dy = temp
	if self.dx ~= 0 then self.dx = -self.dx end
	
	self.dp = 1
end

function Walker:correct()
	local did = false
	for i=1,4 do
		did = false
		if self.dx > 0 and self.x == 39 then self:turn() did = true end
		if self.dy > 0 and self.y == 23 then self:turn() did = true end
		if self.dx < 0 and self.x == 0 then self:turn() did = true end
		if self.dy < 0 and self.y == 0 then self:turn() did = true end
		if not did then break end
	end
end

Walker(0,0,1,0):insert()

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