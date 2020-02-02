-- On Load

fm = fm or {}
if not fm.fontsRegistered then
	Services.FontManager:registerFont("dialog","media/SansKleinCut.ttf")
	fm.fontsRegistered = true
end

-- Meh.

local displayString = "responsibilities"
local gm = {}
gm.bg = r(Scene())
bridge:setSceneClearColor(gm.bg,32,0,0,255)
gm.textfield = ScreenLabel(displayString,30,"dialog")
local width_at_30 = bridge:labelWidth(gm.textfield)
delete(gm.textfield)
gm.textfield = ScreenLabel(displayString,30 * surface_width/width_at_30,"dialog")

local w = bridge:labelWidth(gm.textfield)
local h = bridge:labelHeight(gm.textfield)
gm.textfield:setPosition((surface_width-w)/2,(surface_height-h)/3)

gm.s = r(Screen())
gm.s:addEntity(gm.textfield)

-- Ent({pressed={test=function () print("TEST") end}}):insert()

-- Set up input handler

down = {} pressed = {}
listeners = {basic = Queue()}
mouseDownAt = nil
mouseAt = nil

class "Input" (EventHandler)
function Input:Input()
	EventHandler.EventHandler(self)
end

function Input:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
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
end

do
	input = Input()

	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_MOUSEMOVE)
	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_MOUSEDOWN)
	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_MOUSEUP)
	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_KEYUP)
end