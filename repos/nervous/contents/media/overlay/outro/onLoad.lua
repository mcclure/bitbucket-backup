-- On Load

au.tone:Stop()

gfx_width = 280
gfx_height = 192
by = 1
base0 = string.byte("0")
started = false

if dos then dos:die() dos = nil end
dos = type_automaton()
dos:insert()
--dos:set_centered(0,11,40,string.format("They weren't... dreams, exactly."))
dos:set_centered(0,4,40,string.format("Your score: %d", gm.score))

dos:set_centered(0,18,40,"Space to play again")
dos:set_centered(0,19,40,"Esc to quit")

-- Set up input handler

down = {} pressed = {}
mouseDownAt = nil
mouseAt = nil

class "Input" (EventHandler)
function Input:Input()
	self:EventHandler()
end

function cdelete(e)
	if e then delete(e) end
end

function Input:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == EVENT_MOUSEMOVE then
			cdelete(mouseAt) mouseAt = inputEvent:getMousePosition()
		elseif e:getEventCode() == EVENT_MOUSEDOWN then
			cdelete(mouseDownAt) mouseDownAt = inputEvent:getMousePosition()
			cdelete(mouseAt) mouseAt = vDup(mouseDownAt)
			down[inputEvent.mouseButton] = true
			pressed[inputEvent.mouseButton] = true
		elseif e:getEventCode() == EVENT_MOUSEUP then
			down[inputEvent.mouseButton] = nil
		elseif e:getEventCode() == EVENT_KEYDOWN then
			down[key] = true
			pressed[key] = true
		elseif e:getEventCode() == EVENT_KEYUP then
			down[key] = nil
		end
	end
end

do
	input = Input()

	Services.Core:getInput():addEventListener(input, EVENT_MOUSEMOVE)
	Services.Core:getInput():addEventListener(input, EVENT_MOUSEDOWN)
	Services.Core:getInput():addEventListener(input, EVENT_MOUSEUP)
	Services.Core:getInput():addEventListener(input, EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, EVENT_KEYUP)
end