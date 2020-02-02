-- On Load

km = {resetat = 60*1}

if dos then dos:die() dos = nil end
dos = type_automaton()
dos:insert()

dos:set_centered(0,10,40,"Based on an illustration")
dos:set_centered(0,12,40,"by Anna Anthropy")

-- Set up input handler
down = {}
pressed = {}
mouseAt = Vector3(0,0,0)
class "Input" (EventHandler)
function Input:Input()
	EventHandler.EventHandler(self)
end

function Input:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		if e:getEventCode() == InputEvent.EVENT_MOUSEMOVE then
			delete(mouseAt)
			mouseAt = inputEvent:getMousePosition()
		elseif e:getEventCode() == InputEvent.EVENT_MOUSEDOWN then
			down[inputEvent.mouseButton] = true
		elseif e:getEventCode() == InputEvent.EVENT_MOUSEUP then
			down[inputEvent.mouseButton] = nil
		elseif e:getEventCode() == InputEvent.EVENT_KEYDOWN then
			local key = inputEvent:keyCode()
			pressed[key] = true
		end
	end
end

do
	input = Input()

	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_MOUSEMOVE)
	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_MOUSEDOWN)
	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_MOUSEUP)
	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_KEYDOWN)
end