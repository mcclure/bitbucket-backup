-- On Load

pull(km, {vzero = Vector3(0,0,0), movef = 100, blurbase = 0.00390625})
pull(gm, {physics = PhysicsScreen(screen()), color=Color(0,0,0,1), thue = 0})

gm.physics:setGravity(km.vzero)

-- Set up "spotlight" effect
screen():setScreenShader("FilterMaterial")

shaders = {}
for i=1,2 do
	table.insert(shaders,  shaderBindings(screen(), 
		{blurSize=km.blurbase, brightness=1.02}, i-1) )
end

gm.p = ScreenMesh(ScreenEntity(id("focus")))

local bgtexture = bridge:findOutTexture("FilterMaterial", "mid2")
gm.mirror = ScreenShape(ScreenShape.SHAPE_RECT, surface_width, surface_height)
gm.mirror:setPosition(surface_width/2, surface_height/2)
gm.mirror:setTexture( bgtexture )
screen().rootEntity:addFarChild(gm.mirror)

local machinespec = bridge:loadTableFromFile("media/machines/LOVELYCHAOS.tau")
gm.machine = Machine(machinespec)
gm.sndcontroller = SndController({machine=gm.machine}):insert()

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

--	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEMOVE)
--	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEDOWN)
--	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEUP)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYUP)
end