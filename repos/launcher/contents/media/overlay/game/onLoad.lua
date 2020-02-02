-- On Load

killDos()
dos = type_automaton()
dos:insert()

menu = {
	{"cs1"},
	{"diabolus", "Devil's Chord"},
	{"sunsets", "Sun Sets"},
	{"rainbow", "HOWLER?"},
}

strings = {"Sweet Nothings",""}
for i,v in ipairs(menu) do
	if v[2] then
		table.insert(strings, string.format("%d. %s", i-1, v[2])) -- Notice we shift down for hidden 0
	end
end
table.insert(strings,"")
table.insert(strings,"Please select")

maxlen = 0
for i,v in ipairs(strings) do
	local vl = #v
	if vl > maxlen then maxlen = vl end
end

typetop = (24 - (#strings*2-1))/2 - 1
typeleft = (40 - maxlen)/2

for i,v in ipairs(strings) do
	local top = i == #strings
	
	if i == 1 or i == 2 or top then
		dos:set_centered(0,typetop + i*2 - (top and 0 or 1),40,v)
	else
		local mi = menu[i-2]
		local left = typeleft
		local top = typetop + i*2 - 1
		if mi then mi.left, mi.top = left, top end
		dos:set(left, top,v)
	end
end

if nil then for i=1,24 do -- Grid
	dos:set(30,i-1,string.format("%d",i))
end end

if au then
	for k,v in pairs(au) do
		if type(v) == "table" and v.__delete then delete(v) end
	end
	au = nil
end

loadmessage = nil

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