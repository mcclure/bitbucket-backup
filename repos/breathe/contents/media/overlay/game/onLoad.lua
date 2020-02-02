-- On Load
memory_setup()

-- Graphics

screen():setScreenShader("FilterMaterial")

function bindingFor(s, default)
	if not default then default = 0 end
	local b = screen():getScreenShaderMaterial():getShaderBinding(0)
	if "table" == type(default) then
		b:addLocalParamVector3(s, default);
	else
		b:addLocalParamNumber(s, default);
	end
	local p = b:getLocalParamByName(s)
	if "table" == type(default) then
		p:setVector3(default)
	else
		p:setNumber(default)
	end
	return p
end
shadname = {"px", "py"}
shadset = {}
shadlast = {}
--px = 1
--while px < surface_width or px < surface_height do px = px * 2 end
for i,s in ipairs({1/surface_width, 1/surface_height}) do shadlast[shadname[i] ] = s end

for i,s in pairs(shadname) do  shadset[s] = bindingFor(s, shadlast[s]) end
--]]

-- Input

if not down then down = {} end
moveby = surface_height * 0.01
spinby = -2

class "Keyer" (EventHandler)
function Keyer:Keyer()
	self:EventHandler()
end

function Keyer:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == EVENT_KEYDOWN then
			down[key] = true
		elseif e:getEventCode() == EVENT_KEYUP then
			down[key] = false
		end
	end
end

do
	keyer = Keyer()

	Services.Core:getInput():addEventListener(keyer, EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(keyer, EVENT_KEYUP)
end

-- Player

function dataFor(name)
	local e = id(name)
	if not e then return nil end
	local i = ScreenEntity(id(name))
	return {i:getPosition(), i:getWidth(), i:getHeight()}
end

playerinfo = dataFor("player")
if playerinfo then
	player = ScreenImage("media/colorwheel.png")
	vSetPosition(player, playerinfo[1])
	player:setPositionMode(POSITION_CENTER)
	player:setScale(playerinfo[2]/player:getImageWidth(), playerinfo[3]/player:getImageHeight())
	screen():addChild(player)
	screen():removeChild(id("player"))
end

function insertImage(e, i)
	local einfo = dataFor(e)
	local ne = ScreenImage(i)
	vSetPosition(ne, einfo[1])
	ne:setPositionMode(POSITION_CENTER)
	ne:setScale(einfo[2]/ne:getImageWidth(), einfo[3]/ne:getImageHeight())
	screen():addChild(ne)
	screen():removeChild(id(e))
	return(ne)
end

function removethese(ents)
	for i,e in ipairs(ents) do
		screen():removeChild(id(e))
	end
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