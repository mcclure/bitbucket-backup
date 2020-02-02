-- On Load

function removeone(e)
	if e then
		bridge:room_remove_screen(e)
		delete(e)
	end
end

function removethese(ents)
	for i,e in ipairs(ents) do
		removeone(id(e))
	end
end

-- Graphics

screen():setScreenShader(rule30 and "FilterMaterial30" or "FilterMaterial")
shader = shaderBindings(screen(),
                       {px=1/surface_width,py=1/surface_height,thresh=0})

-- Input

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

-- Player

function dataFor(name)
	local e = id(name)
	if not e then return nil end
	local i = ScreenEntity(id(name))
	return {i:getPosition(), i:getWidth(), i:getHeight()}
end

player = nil
playerinfo = dataFor("player")
if playerinfo then
	player = ScreenImage("media/colorwheel.png")
	vSetPosition(player, playerinfo[1])
	player:setPositionMode(ScreenEntity.POSITION_CENTER)
	player:setScale(playerinfo[2]/player:getImageWidth(), playerinfo[3]/player:getImageHeight())
	screen():addChild(player)
	removeone(id("player"))
end

function insertImage(e, i)
	local einfo = dataFor(e)
	local ne = ScreenImage(i)
	vSetPosition(ne, einfo[1])
	ne:setPositionMode(ScreenEntity.POSITION_CENTER)
	ne:setScale(einfo[2]/ne:getImageWidth(), einfo[3]/ne:getImageHeight())
	screen():addChild(ne)
	screen():removeChild(id(e))
	return(ne)
end

function loadnext(name)
	bridge:load_room(string.format("media/overlay/game,media/overlay/sound,media/overlay/%s,media/bg.svg,media/%s.svg", name, name))
end

function load_next_if(ename, rname)
	if collide then
		local cname = bridge:room_name(collide)
		if cname == ename then
			loadnext(rname)
			return true
		end
	end
	return false
end

function special()
	local is_special = down[KEY_LSHIFT] or down[KEY_RSHIFT] or down[KEY_RCTRL] or down[KEY_LCTRL] or down[KEY_RALT] or down[KEY_LALT] or down[KEY_RMETA] or down[KEY_LMETA]
	if special_reverse then is_special = not is_special end
	return is_special
end