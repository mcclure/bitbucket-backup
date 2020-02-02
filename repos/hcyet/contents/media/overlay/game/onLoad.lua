-- On Load
km = {}
gm = {}

local s = r(Screen())

for i=1,100 do
	math.random() -- stupid lua bug
end

gm.model = MarkovModel(10000)
gm.model:loadTrainingFile("media/dictionary.txt")

km.alphabet = {"a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"}

function isin(a, list)
	for i,v in ipairs(list) do
		if a == v then return true end
	end
	return false
end

gm.current = ""
function nextFollowing(str)
	local options = {}
	for i,v in ipairs(km.alphabet) do
		local check = str .. v
		if gm.model:hits(check)>0 then
			table.insert(options, v)
		end
	end
	local count = #options
	if count > 0 then
		return options[math.random(#options)]
	else
		return nil
	end
end

function nextCurrent()
	local newstr = nextFollowing(gm.current)
	if newstr then
		gm.current = gm.current .. newstr
		return newstr
	end
	return nil
end

function dying(why)
	Services.Renderer:setClearColor(0.0,0.0,0.0,1)
	local left = surface_width/5
	local top = surface_height/8

	gm.label = ScreenLabel(why,30 * surface_height/800)
	gm.label:setColor(1,1,1,1)
	gm.label:setPosition(left,top,0)
	s:addChild(gm.label)

	gm.label = ScreenLabel(string.format("your score: %d", #gm.current-1),30 * surface_height/800)
	gm.label:setColor(1,1,1,1)
	gm.label:setPosition(left,top+surface_height/3,0)
	s:addChild(gm.label)

	gm.label = ScreenLabel("esc to exit",30 * surface_height/800)
	gm.label:setColor(1,1,1,1)
	gm.label:setPosition(left,top+2*surface_height/3,0)
	s:addChild(gm.label)
	
	gm.dead = true
end

function resetCurrent()
	local newstr = nextCurrent()
	if not newstr then dying("forfeit") return end
	if gm.label then
		s:removeChild(gm.label)
		delete(gm.label)
	end
	gm.label = ScreenLabel(newstr,30 * surface_height/800)
	gm.label:setColor(0,0,0,1)
	gm.label:setPosition(math.random(surface_width-30),math.random(surface_height-30),0)
	s:addChild(gm.label)
end

resetCurrent()

-- Set up input handler

down = {} pressed = {}
listeners = { input = Queue() } -- To get unicode-level key input
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
		down[key] = nil
	end
end

do
	input = Input()

	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYUP)
end