-- On Load

Services.Renderer:setClearColor(0.0,0.0,0.0,1)
--Services.Core:enableMouse(false)
	
killDos()
dos = type_automaton()
dos:insert()

strings = {
"Tau.",
--"Advanced Audio Intelligence Generatrix",
--"",
"",
"Press enter to start",
}

maxlen = 0
for i,v in ipairs(strings) do
	local vl = #v
	if vl > maxlen then maxlen = vl end
end

typetop = (24 - (#strings*2-1))/2 - 1
typeleft = (40 - maxlen)/2

for i,v in ipairs(strings) do
	local top = i == #strings
	if top then
		dos:set(typeleft,typetop + i*2,v)
	else
		dos:set(typeleft,typetop + i*2 - 1,v)
	end
end

if nil then for i=1,24 do -- Grid
	dos:set(30,i-1,string.format("%d",i))
end end

-- Set up input handler

down = {}
pressed = {}
listener = Queue()
moveby = surface_height * 0.01
spinby = -2

class "Keyer" (EventHandler)
function Keyer:Keyer()
	EventHandler.EventHandler(self)
end

function Keyer:handleEvent(e)
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == InputEvent.EVENT_KEYDOWN then
			down[key] = true
			pressed[key] = true
			listener:push({key, bridge:charCode(inputEvent)})
		elseif e:getEventCode() == InputEvent.EVENT_KEYUP then
			down[key] = nil
		end
end

do
	keyer = Keyer()

	Services.Core:getInput():addEventListener(keyer, keyer.handleEvent, InputEvent.EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(keyer, keyer.handleEvent, InputEvent.EVENT_KEYUP)
end