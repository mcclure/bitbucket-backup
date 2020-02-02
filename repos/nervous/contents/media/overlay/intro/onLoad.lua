-- On Load

gfx_width = 280
gfx_height = 192
by = 1
base0 = string.byte("0")
started = false

if dos then dos:die() dos = nil end
dos = type_automaton()
dos:insert()
--dos:set_centered(0,11,40,string.format("They weren't... dreams, exactly."))
dos:set_centered(0,4,40,"The Nervous System")
dos:set_centered(0,5,40,"(Engine test 1)")

dos:set(0,8,"Controls: Arrow keys")
dos:set(0,9,"Esc to give up")
dos:set(0,12,"Walls wrap.")
dos:set(0,13,"If you can't hear the bassline, turn")
dos:set(0,14,"up your sound.")
dos:set(0,15,"Right now this is basically just Snake.")
dos:set(0,18,"KOTM 10/20/12")
dos:set(0,19,"Space to start")

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