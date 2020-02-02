-- On Load

km = { step = 16, wallh = 0.75, wallres = 1, shotw = 0.1, shotdown = 0.1, winat = 4, floors=false }
cm = { angle = 0, camera_height_mod = 1.25 }
gm = { monster = {}, ent = {} }
gm.p = MovablePlayer():insert()

local levels = { -- 1.6, 2.0, 3.2, 6.4, 10
	{ monstercount = 2, pixel = 1.6*0.01 },
	{ monstercount = 3, pixel = 2.0*0.01 },
	{ monstercount = 3, pixel = 3.2*0.01 },
	{ monstercount = 4, pixel = 6.4*0.01 },
	{ monstercount = 4, pixel = 10*0.01 },
}
lm = levels[fm.level] or levels[#levels]

-- Meh.

local s = scene()

s:getDefaultCamera():setPostFilter("FilterMaterial")
shader = shaderBindings(s:getDefaultCamera(),
                       {aspect=(surface_height/surface_width), span=lm.pixel})
s:getDefaultCamera().frustumCulling = false


killDos()
dos = type_automaton()
dos:insert()

function score(x)
	fm.score = x
	dos:clear()
	local s = tostring(x)
	dos:set(40-#s,23,s)
end
score(fm.score or 0)

function doDeath()
	stopall()
	au.death:Play(false)
	gm.m.theme:makeSprite(gm.p:pos(), {sprite="media/sprite/714.png"})
	cm.debugLook = true
	fm.lives = fm.lives - 1
	local message = (fm.lives >= 1) and string.format("%d LIFE",fm.lives) or "YOUR GAME OVER"
	dos:set(40-#message,0,message)
	dos:set(0,0,"THE QUEEN DEFEATED")
	dos:set(0,23,"SPACE KEY CONTINUE")
end

function doKill(m)
	m:die()
	au.kill:Play(false)
	gm.m.spawn.pop = gm.m.spawn.pop - 1
	score(fm.score + 100)
	fm.kills = fm.kills + 1
	if fm.kills > km.winat then
		bridge:load_room_txt("media/noface.txt")
	end
end

local g = GridFrom(lm.filename or "media/level/1.txt")
gm.m = LevelMap({map=g})
gm.m:add()

gm.p.x = gm.m.entry.x
gm.p.y = gm.m.entry.y

local upright = r(Vector3(0,1,0))
function updateCamera() -- Call on each turn, manages... camera.
	local camera = s:getDefaultCamera()
	local angle = gm.p:orient() * 90
	local pos = gm.p:pos()
	local at = a(Vector3(pos.x, 0, pos.y))
	for i,camera in ipairs({s:getDefaultCamera()}) do
		if not cm.debugLook then
			camera:setPosition(at.x, at.y+cm.camera_height_mod, at.z)
			camera:setYaw(angle)
			camera:setPitch(0)
			camera:setRoll(0)
		else
			local offs = a(Vector3(0,10,10))
			local at2 = vAdd(at, offs)
			camera:setPosition(at2.x,at2.y,at2.z)
			camera:lookAt(at,upright)
		end
	end
end
updateCamera()

-- Set up input handler

down = {} pressed = {}
listeners = {} -- To get unicode-level key input
mouseDownAt = nil
mouseAt = nil
function repeatok() return cm.debugLook end

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
		if repeatok() or not down[key] then
			down[key] = true
			pressed[key] = true
		end
		for k,v in pairs(listeners) do
			v:push({key, bridge:charCode(inputEvent)})
		end
	elseif e:getEventCode() == InputEvent.EVENT_KEYUP then
		down[key] = nil
	end
end

do
	input = Input()

--	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEMOVE)
--	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEDOWN)
--	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEUP)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYUP)
end