-- On Load

km = {ddz = 0.001, dxy = 0.05, dsz = 1, dslimit = 10, refract=15, spawnz = 1, free = 4, back=100, player_out=10, lives = 3,}
gm = {dz = 1.5, s = {}, lastshot = -km.refract*2, lastspawn = -km.spawnz*2, e = Queue(), da = {}, de = {}, life=km.lives, score=0, mode= -1}
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
		for i,v in ipairs(gm.s) do
			local pz = a(gm.p:getPosition()) pz.x = 0 pz.y = 0
			vSetPosition(v.s, vAdd(pz, v.sat))
		end
	end
end

function reset_player()
	gm.p = ScenePrimitive(TYPE_BOX, 1.0,1.0,1.0) -- Player
	gm.pat = r(Vector3(0,0,km.player_out))
	place_player()
	gm.p:setColor(1,0.5,1,1)
	gm.p:setMaterialByName("CubeMaterial")
	s:addEntity(gm.p)
end

function make_shot()
	local n = {s = bridge:slbv(a(Vector3(0,0,0)), a(Vector3(0,0,5)), 10), sat=Vector3(gm.pat.x,gm.pat.y,0)}
	n.s:setColor(0,0,0,1)
	s:addEntity(n.s)
	table.insert(gm.s, n)
end

function start_game()
	if gm.p then
		s:removeEntity(gm.p)
		delete(gm.p)
	end
	reset_player()
	pull(gm, {mode=1, dz=0.5, deaden=false, realdead=false, score=0, life=km.lives,
	})
	dos:clear()
	dosScore()
	dosLife()
	s_ow:setPitch(1)
	s_tart:Play(false)
end

reset_player()

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

-- SOUNDS

have_audio = false
if not have_audio then
	have_audio = true
	
	-- Generate shoot sound
	if s_hoot then delete(s_hoot) s_hoot = nil end
	do
		local data = a(NumberArray())
		local limit = (512*32)-1
		local w = 20
		local v
		for i=0,limit do
			v = ((math.floor(i/w)%2)==0) and 0.25 or -0.25
			if i % 20 == 0 then w = w + 1 end
			data:push_back(v*((limit-i)/limit))
		end
		s_hoot = bridge:soundFromValues(data)
	end
	
	-- Generate shoot sound
	if s_tart then delete(s_tart) s_tart = nil end
	do
		local data = a(NumberArray())
		local limit = (512*32)-1
		local w = 20
		local v
		for i=0,limit do
			v = ((math.floor(i/w)%2)==0) and 0.25 or -0.25
			data:push_back(v*((limit-i)/limit))
		end
		s_tart = bridge:soundFromValues(data)
	end
	
	-- Generate kill sound
	if s_kill then delete(s_kill) s_kill = nil end
	do
		local data = a(NumberArray())
		local limit = (512*32)-1
		local w = 20
		local v
		for i=0,limit do
			v = (math.random() > 0.5) and 0.25 or -0.25
			--if i % 20 == 0 then w = w + 1 end
			data:push_back(v*((limit-i)/limit))
		end
		s_kill = bridge:soundFromValues(data)
	end
	
	if s_ow then delete(s_ow) s_ow = nil end
	do
		local data = a(NumberArray())
		local limit = (512*32)-1
		local w = 20
		local v
		for i=0,limit do
			v = ((math.floor(i/w)%2)==0) and 0.25 or -0.25
			if i % 80 == 0 then w = w + 1 end
			data:push_back(v*((limit-i)/limit))
		end
		s_ow = bridge:soundFromValues(data)
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