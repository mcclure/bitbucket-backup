-- On Load

if not om then om = {highscore = 0} end
km = {j = 0.5, ddz = 0.015, dxy = 0.05, dsz = 1, dslimit = 10, refract=15, spawnz = 1, free = 4, back=100, player_out=10, lives = 3,}
gm = {dz = 0, lastspawn = -km.spawnz*2, e = Queue(), da = {}, life=km.lives, score=0, mode= -1,
}
cm = {}

-- Set up "spotlight" effect
--screen():setScreenShader("FilterMaterial")
--shader = shaderBindings(screen(), 
--	{center=Vector3(0.1,0.1,0.0), radius=0.1, intensity=100.0, aspect=(surface_width/surface_height)})

-- GAME

dos = type_automaton()
dos:insert()

local s = scene()

do -- Make camera
	cm.cam = s:getDefaultCamera()
	cm.z = 0
	s:getDefaultCamera():setPosition(0,0,0)
	s:getDefaultCamera():lookAt(a(Vector3(0,0,1)),a(Vector3(0,1,0)))
end

function place_player()
	s:getDefaultCamera():setPosition(0,0,cm.z)
	if gm.mode > 0 then
		vSetPosition(gm.p, vAdd(a(s:getDefaultCamera():getPosition()), gm.pat))
	end
end

function reset_player(x,y)
	gm.p = ScenePrimitive(TYPE_BOX, 1.0,1.0,1.0) -- Player
	gm.pat = r(Vector3(x,y,km.player_out))
	place_player()
	gm.p:setColor(1,0.5,1,1)
	gm.p:setMaterialByName("CubeMaterial")
	s:addEntity(gm.p)
end

function start_game()
	if gm.p then
		s:removeEntity(gm.p)
		delete(gm.p)
	end
	
	local x,y = 0,0
	for i,e in ipairs(gm.da) do
		local at = a(e.e:getPosition())
		if e.z > cm.z+km.player_out+1 then
			x = at.x
			y = at.y
			break
		end
	end
	
	reset_player(x,y)
	pull(gm, {mode=1, dz=0.5, deaden=false, realdead=false, score=0, life=km.lives, lastland = cm.z, highscore=0
	})
	dos:clear()
	dosScore()
	dosLife()
	s_ow:setPitch(1)
	s_tart:Play(false)
end

reset_player(0,0)

function dosScore()
	dos:set(0,23,string.format("%05d",gm.score))
end

function dosLife()
	local start = 39-km.lives
	for i=1,km.lives do
		local set_to = (i <= gm.life) and "*" or " "
		dos:set(start+i,23,set_to)
	end
end

-- Set up input handler
down = {} pressed = {}
class "Input" (EventHandler)
function Input:Input()
	self:EventHandler()
end

function Input:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == EVENT_KEYDOWN then
			down[key] = true
			pressed[key] = true
		elseif e:getEventCode() == EVENT_KEYUP then
			down[key] = false
		end
	end
end

do
	input = Input()
	
	Services.Core:getInput():addEventListener(input, EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, EVENT_KEYUP)
end