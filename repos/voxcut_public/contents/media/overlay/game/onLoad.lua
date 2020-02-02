-- On Load

gfx_width = 280
gfx_height = 192

dos = freetype_automaton(true)
dos:insert()
dos:set_centered(0,11,40,string.format("They weren't... dreams, exactly."))
dos.g = dos:toGfx()
for x=0,gfx_width-1 do for y=0,(gfx_height-1)/2 do
dos.g:pxset(x,y,(x+y)%2==0)
end end
dos.g:broken_pxcopy(dos.g, 0, gfx_height/2, 0,0, gfx_width/2, gfx_height/2)

-- "Humming noise" why is this part of this demo?

onepass = (gfx_height/2)/60
started = false
function clamp(low, v, high)
	if low > v then return low end
	if high < v then return high end
	return v
end
do
	local data = a(NumberArray())
	local limit = onepass * 44100 -- 16384*4
	local last = 0
	for i=0,limit do
		local v = math.random() * 4 - 2
		v = (v + last*9)/10
		last = v
		
		data:push_back(clamp(-1, v, 1))
	end
	humEffect = r(bridge:soundFromValues(data))
	humEffect:Play(true)
end
do
	local data = a(NumberArray())
	local limit = onepass * 44100
	local last = 0
	for i=0,limit do
		local v = ( (math.floor(i/(44100/110 * sqrt(2)))%2) == 1 ) and 1 or -1
		v = (v + last*9)/10
		last = v
		
		data:push_back(clamp(-1, v * i/limit, 1))
	end
	humEffect2 = r(bridge:soundFromValues(data))
end

-- Set up input handler
down = {}
pressed = {}
mouseAt = Vector3(0,0,0)
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