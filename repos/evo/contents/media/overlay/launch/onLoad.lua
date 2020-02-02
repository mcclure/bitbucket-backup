-- Launch, On Load

um = {base = userSettings.base or "http://vote.grumpybumpers.com:9999"}
pull(um, {
	welcome=userSettings.welcome or www("/welcome"),
})
bridge:coreServices().drawScreensFirst = true
-- TODO: um.force?

player_info = bridge:loadTableFromFile("player.xml", true)
if (not player_info) or "table" ~= type( player_info ) or not player_info.id then
	math.random(0,2147483646) math.random(0,2147483646) math.random(0,2147483646) -- WAKE UP!! WAKE UP!!
	player_info = {}
	player_info.id = string.format("p%s", math.random(0,2147483646))
	bridge:saveTableIntoFile("player.xml", "player", player_info);
	if _DEBUG then print({"PLAYER ID: ", player_info.id}) end
end

-- Start

makeDos()
dosLoading()
xhr(um.welcome, function(t)
	dos:clear()
	dos:set_centered(0,0,40,"The World Hates You. http://runhello.com")
	fill(dos, 0, 1,  40, 1, "-")
	fill(dos, 0, 22, 40, 1, "-")
	dos:set_centered(0,23,40,"Remember to walljump!  Press \010 to start.")
	local x,y = 0,3
	if t.message then
		for c in t.message:gmatch(".") do
			if c == "\n" then
				x = 0 y = y + 1
			else
				dos:set(x,y,c)
				x = x + 1
				if x >= 40 then
					x = 0 y = y +1
				end
			end
		end 
	end
	pull(um, t.um) -- TODO: TEST
end)

-- Set up input handler
down = {} pressed = {}
mouseAt = Vector3(0,0,0)
class "Input" (EventHandler)
function Input:Input()
	self:EventHandler()
end

function Input:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == EVENT_MOUSEMOVE then
			delete(mouseAt)
			mouseAt = inputEvent:getMousePosition()
		elseif e:getEventCode() == EVENT_MOUSEDOWN then
			down[inputEvent.mouseButton] = true
		elseif e:getEventCode() == EVENT_MOUSEUP then
			down[inputEvent.mouseButton] = nil
		elseif e:getEventCode() == EVENT_KEYDOWN then
			down[key] = true
			pressed[key] = true
		elseif e:getEventCode() == EVENT_KEYUP then
			down[key] = false
		end
	end
end

do
	input = Input()

	Services.Core:getInput():addEventListener(input, EVENT_MOUSEMOVE)
	Services.Core:getInput():addEventListener(input, EVENT_MOUSEDOWN)
	Services.Core:getInput():addEventListener(input, EVENT_MOUSEUP)
	Services.Core:getInput():addEventListener(input, EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, EVENT_KEYUP)
end