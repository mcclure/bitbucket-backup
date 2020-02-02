-- Title screen setup
titleDone = false

-- Cube
local titlecube = r(Scene())
local camera = titlecube:getDefaultCamera()
cube = ScenePrimitive(TYPE_BOX, 1, 1, 1)
cube:setPosition( 0, 0, 0 )
titlecube:addEntity(cube)
cube:setColor(1,1,1,1)
cube:setMaterialByName("GroundMaterial")
camera:setPosition(-1.5,0,1)
camera:lookAt(a(Vector3(0,0,1)),a(Vector3(0,1,0)))

-- Text
dos = type_automaton()
dos:insert()

dos_write = {
	{ 0, 0,   6,  "7DRL", },
	{ 0, nil, 10, "A~game~by~Andi~McClure", },
	{ 0, nil, 11, "Music~by~Liz~Ryerson" },
	{ 0, nil, 12, "Controls:~WASD~+~Mouse" },
	{ 0, nil, 14, "+/-~to~adjust~mouse~sensitivity" },
	{ 0, nil, 18, "Press~a~button", },
}

function toggle(t)
	if t[2] then
		t[1] = ticks + 2
	else
		t[1] = ticks + math.random(2*60)
	end
	t[2] = not t[2]
end

-- Keyboard handler
class "TitleInput" (EventHandler)
function TitleInput:TitleInput()
	self:EventHandler()
end

function TitleInput:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		if e:getEventCode() == EVENT_MOUSEMOVE then
		elseif e:getEventCode() == EVENT_KEYDOWN then
			titleDone = true
		end
	end
end

do
	input = TitleInput()

	Services.Core:getInput():addEventListener(input, EVENT_KEYDOWN)
end