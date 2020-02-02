-- Game on load

-- Set up input handler
down = {}
mouseAt = Vector3(0,0,0)
class "Input" (EventHandler)
function Input:Input()
	self:EventHandler()
end
bridge:coreServices().drawScreensFirst = true

function Input:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		if e:getEventCode() == EVENT_MOUSEMOVE then
			delete(mouseAt)
			mouseAt = inputEvent:getMousePosition()
		elseif e:getEventCode() == EVENT_MOUSEDOWN then
			down[inputEvent.mouseButton] = true
		elseif e:getEventCode() == EVENT_MOUSEUP then
			down[inputEvent.mouseButton] = nil
		end
	end
end

do
	input = Input()

	Services.Core:getInput():addEventListener(input, EVENT_MOUSEMOVE)
	Services.Core:getInput():addEventListener(input, EVENT_MOUSEDOWN)
	Services.Core:getInput():addEventListener(input, EVENT_MOUSEUP)
end