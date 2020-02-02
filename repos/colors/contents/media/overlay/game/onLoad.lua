-- On Load

-- DON'T FORGET TO COMPILE THIS ONE WITH -O3!

pull(gm, {r=Router():insert()})

Services.Renderer:setClearColor(0,0,0,0)

killDos()
dos = type_automaton()
dos:insert()
dos.g = dos:toGfx()

for i,v in ipairs({"r","g","b"}) do
	local ls = LevelScreen({channel=v}):insert()

	for x=1,17 do for y=1,10 do
		if x==1 or x==17 or y==1 or y==10 then
			ls.wall:set(x,y,1)
		end
	end end
	ls.wall:set(5*i, 5, 1)

	local s = FixedController({pos=P(8,3), live=ls, basis="block-target", passable=true}):insert()

	local d = DoorController({pos=P(9,10), live=ls}):insert()

	local b = BlockController({pos=P(4*i, 7), live=ls}):insert()
	
	d.watch={b} d.target=s
	
	ls.wall:set(d.pos.x, d.pos.y, nil)

	local teststrings = {"Inspired by Anna Anthropy   ",
						 "Inspired by Michael Brough  ",
						 "Inspired by Stephen Lavelle ",}
	local test = Ent({onTick = function (self)
		blittext(ls.g, 0,21,"---------------------------------------!")
		blittext_centered(ls.g, 0, 22, 40, teststrings[i])
		dos:set(0,23,"---------------------------------------!")
	end}):insert()

	ls:reblank()

	local control = PlayerController({pos=P(2,2),live=ls}):insert()
end

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

	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEMOVE)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEDOWN)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEUP)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYUP)
end