x = ScreenLabel("FAIL",24,"Wide")
x:setPosition(surface_width/4,surface_height/4-line_height)
screen():addChild(x)

x = ScreenLabel("PRESS BUTTON",24,"Wide")
x:setPosition(surface_width/4,3*surface_height/4)
screen():addChild(x)

snd_die = Sound("media/die.ogg")
snd_die:setVolume(0.5)
snd_die:Play(false)

-- Keyboard handler
class "Keyer" (EventHandler)
function Keyer:Keyer()
	self:EventHandler()
end

ready = false

function Keyer:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == EVENT_KEYDOWN then
			ready = true
		elseif e:getEventCode() == EVENT_KEYUP and ready then
			bridge:load_room("media/overlay/game")
		end
	end
end

do
	local keyer = Keyer()

	Services.Core:getInput():addEventListener(keyer, EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(keyer, EVENT_KEYUP)
end