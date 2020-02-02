-- On Update

if _DEBUG and pressed[KEY_RETURN] then
	gm.paused = not gm.paused
	print({"MEH!", gm.paused})
end
if gm.paused then pressed = {} return end

local s = scene()

if gm.mode > 0 and not gm.deaden then
	gm.dz = gm.dz + km.ddz
else
	if gm.mode <= 0 then
		
	elseif gm.realdead then
		gm.dz = gm.dz * 0.95
		gm.deaden = gm.deaden + 0.05
		local deaden = clamp(0,gm.deaden,1.0)
		gm.p:setColor(0,0,0,1.0-gm.deaden)
		s_ow:setPitch(1.0-gm.deaden)
		if gm.dz < 0.0001 then
			gm.dz = 0
			gm.mode = 0
			dos:set(35,23,"SPACE")
		end
	else
		gm.dz = gm.dz + 0.015
		gm.deaden = gm.deaden + 0.05
		local deaden = clamp(0,gm.deaden,1.0)
		gm.p:setColor(deaden,deaden/2,deaden,1.0)
		if gm.dz >= 0.5 then
			gm.deaden = nil
		end
	end
end
cm.z = cm.z + gm.dz

if down[KEY_LEFT] then
	gm.pat.x = gm.pat.x + km.dxy
elseif down[KEY_RIGHT] then
	gm.pat.x = gm.pat.x - km.dxy
end
if down[KEY_UP] then
	gm.pat.y = gm.pat.y + km.dxy
elseif down[KEY_DOWN] then
	gm.pat.y = gm.pat.y - km.dxy
end

gm.pat.x = clamp(-km.free,gm.pat.x,km.free)
gm.pat.y = clamp(-km.free,gm.pat.y,km.free)

local ns = {}
for i,v in ipairs(gm.s) do
	v.sat.z = v.sat.z + km.dsz
	if v.sat.z > km.back then
		s:removeEntity(v.s)
		delete(v.s)
		delete(v.sat)
	else
		table.insert(ns, v)
	end
end
gm.s = ns

if cm.z - gm.lastspawn >= km.spawnz then
	local n = {z=cm.z+km.back,r=random()*0.5+0.5} -- Enemy
	if math.random() < 0.2 then
		n.e = ScenePrimitive(TYPE_BOX, 1.0,1.0,1.0)
		n.e:setColor(n.r,0,0,1)
		n.e:setMaterialByName("CubeMaterial")
		table.insert(gm.da, n)
	else
		n.e = bridge:slbv(a(Vector3(0,0,0)), a(Vector3(0,0,-5)), 1)
		n.e:setColor(0,0,0,1)
	end
	n.e:setPosition(math.random()*km.free*2-km.free,math.random()*km.free*2-km.free,n.z)
	s:addEntity(n.e)
	gm.lastspawn = cm.z
	gm.e:push(n)
end
while gm.e:peek() and gm.e:peek().z < cm.z do
	local e = gm.e:pop()
	s:removeEntity(e.e)
	delete(e.e)
end
local nda = {}
for i,e in ipairs(gm.da) do
	local at = a(e.e:getPosition()) at.z = at.z - (cm.z + km.player_out)
	local x,y,z = at.x,at.y,at.z
	if e.z >= cm.z then
		local crash = false
		for i,v in ipairs(gm.s) do
			if math.abs(x-v.sat.x) < 1 and math.abs(y-v.sat.y) < 1 and math.abs(z-v.sat.z) < 1 and not v.dead then
				s:removeEntity(v.s)
				v.dead = true
				crash = true
				s_kill:Play(false)
				gm.score = gm.score + 10
				dosScore()
				break
			end
		end
		if crash then
			e.dead = 1.0
			table.insert(gm.de, e)
		else
			z = z + km.player_out
			if math.abs(x-gm.pat.x) < 1 and math.abs(y-gm.pat.y) < 1 and math.abs(z-gm.pat.z) < 1 and gm.mode>0 and not gm.deaden then
				gm.deaden = 0.0
				s_ow:Play(false)
				gm.life = gm.life - 1
				dosLife()
				if gm.life > 0 then
					gm.dz = -0.5
				else
					gm.realdead = true
				end
			end
		
			table.insert(nda, e)
		end
	end
end
gm.da = nda

local nde = {}
for i,e in ipairs(gm.de) do
	if e.z >= cm.z then
		if e.dead < 0 then
			s:removeEntity(e.e)
		else 
			e.e:setColor(0,0,0,e.dead)
			e.dead = e.dead - 0.1
			table.insert(nde, e)
		end
	end
end
gm.de = nde

if pressed[KEY_SPACE] then
	if gm.mode == 0 then
		start_game()
	elseif (ticks-gm.lastshot>km.refract) and not gm.realdead and gm.mode > 0 then
		s_hoot:Play(false)
		make_shot()
		gm.lastshot = ticks
	end
end

if gm.mode < 0 and cm.z > 100 then
	gm.mode = 0
	dos:set(0,0,"FALL")
	dos:set(29,0,"KOTM#9/8/12")
	dos:set_centered(0,23,40,"CONTROLS:#\008#\021#\010#\011#SPACE")
	gm.dz = 0
end

pressed = {}
place_player()