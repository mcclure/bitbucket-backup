-- On Load

if not fm then
	fm = {}
	player_info = bridge:loadTableFromFile("score.xml", true)
	fm.maxscore = player_info and player_info.maxscore
	
	Services.Core:enableMouse(false)
end

if not au then
	au = {}
	for i,v in ipairs({"death","face","kill","rope","spawn","escape"}) do
		au[v] = Sound(string.format("media/sound/%s.ogg",v))
	end
end

function stopall()
	for k,v in pairs(au) do
		v:Stop()
	end
end

killDos()
dos = type_automaton()
dos:insert()

if fm.score and fm.score > 0 and not (fm.maxscore and fm.maxscore > fm.score) then
	fm.maxscore = fm.score
	if not player_info or "table" ~= type( player_info ) then
		player_info = {}
	end
	player_info.maxscore = fm.maxscore
	bridge:saveTableIntoFile("score.xml", "player", player_info);
end

local scoreline = ""
if fm.maxscore then
	scoreline = string.format("HIGH SCORE: %d", fm.maxscore)
end

strings = {
"LESBIAN SPIDER MARS QUEENS",
"",
"",
"(C) 1992 SVEIKI ATVYKE",
"SPACE KEY START",
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
	if 1 then
		dos:set_centered(0,typetop + i*2,40,v)
	else
		dos:set(typeleft,typetop + i*2 - 1,v)
	end
end

dos:set_centered(0,21,40,scoreline)

if nil then for i=1,24 do -- Grid
	dos:set(30,i-1,string.format("%d",i))
end end

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