-- On Load

needing = nil
color = false

killDos()
dos = type_automaton()
dos:insert()

dos:set_centered(0,2,40,"Sun Sets")

dos:set(2,7,"* Use arrow keys.")
dos:set(2,9,string.format("* Use %s + arrow keys.","shift"))
dos:set(2,11,"* Type words.")
dos:set(2,13,"* Do not be Icarus.")

gcline = 19
dos:set_centered(0,19,40,"Select: (G)rayscale (C)olor")

dos:set_centered(0,21,40,"Select to start:")
dos:set_centered(0,23,40,"Mode (1), (2) or (3)")

-- Input

down = {}
pressed = {}
listener = Queue()

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
