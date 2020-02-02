-- Standard input handler -- On Load

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
		cdelete(mouseAt) mouseAt = inputEvent:getMousePosition()
	elseif e:getEventCode() == InputEvent.EVENT_MOUSEDOWN then
		cdelete(mouseDownAt) mouseDownAt = inputEvent:getMousePosition()
		cdelete(mouseAt) mouseAt = vDup(mouseDownAt)
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

adhocui = true

if adhocui then

uiscreen = r(Screen())

if false then -- No mouse needed
uinet = r(ScreenEntity())
uinet.processInputEvents = true
uinet.visible = false
--uinet:setPositionMode(ScreenEntity.POSITION_TOPLEFT)
uinet:setPosition(surface_width/2,surface_height/2)
uiscreen:addChild(uinet)
uinet:setHitbox(surface_width,surface_height)
uinet:addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEMOVE)
uinet:addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEDOWN)
uinet:addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEUP)
end

end