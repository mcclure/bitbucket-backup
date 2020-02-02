-- On Update

if _DEBUG and pressed[KEY_RSHIFT] then
	gm.paused = not gm.paused
	print({"MEH!", gm.paused})
end
if gm.paused then pressed = {} return end

local s = scene()
local was_grounded = gm.grounded
gm.grounded = nil

if gm.mode ~= 0 and not gm.realdead and not was_grounded then
	gm.dz = gm.dz + km.ddz
end
if gm.deaden then
	if gm.mode <= 0 then
		
	elseif gm.realdead then
		gm.deaden = gm.deaden + 0.05
		local deaden = clamp(0,gm.deaden,1.0)
		gm.p:setColor(0,0,0,1.0-gm.deaden)
		s_ow:setPitch(1.0-gm.deaden)
		if deaden == 1.0 then
			gm.dz = 0
			gm.mode = 0
			if gm.highscore and gm.highscore > 0 then
				dos:set_centered(0,23,40,string.format("HIGH#SCORE:#%05d",gm.highscore))
			end
			dosScore()
			dos:set(35,23,"SPACE")
		end
	else
		gm.deaden = gm.deaden + 0.05
		local deaden = clamp(0,gm.deaden,1.0)
		gm.p:setColor(deaden,deaden/2,deaden,1.0)
		if gm.dz >= 0.5 then
			gm.deaden = nil
		end
	end
end
local last_cmz = cm.z
cm.z = cm.z + gm.dz

if not gm.realdead then
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
end

gm.pat.x = clamp(-km.free,gm.pat.x,km.free)
gm.pat.y = clamp(-km.free,gm.pat.y,km.free)

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
	local at = a(e.e:getPosition())
	local x,y,z = at.x,at.y,at.z - (cm.z + km.player_out)
	if e.z >= cm.z then
		z = z + km.player_out
		if math.abs(x-gm.pat.x) < 1 and math.abs(y-gm.pat.y) < 1 and math.abs(z-gm.pat.z-0.1) < 1 and gm.mode>0 then
			local fellfrom = at.z-last_cmz-km.player_out
			if fellfrom > 0.9 then
				if gm.dz > 1 and not gm.realdead then
					gm.deaden = 0.0
					gm.life = gm.life - math.floor(gm.dz)
					dosLife()
					if gm.life > 0 then
						s_louch:Play(false)
					else
						s_ow:Play(false)
						s:removeEntity(gm.p)
						s:addEntity(gm.p)
						gm.realdead = true
						if om.highscore > 0 then gm.highscore = om.highscore end
						if om.highscore < gm.score then om.highscore = gm.score end
					end
				else
					if not was_grounded then
						s_land:Play(false)
					end
				end
				
				gm.dz = 0
				gm.grounded = e
				
	--			print({ticks,cm.z,z-gm.pat.z,cm.z - (1-(z-gm.pat.z)))
				cm.z = cm.z - (1-(z-gm.pat.z))
				if not gm.realdead then gm.score = cm.z - gm.lastland end
					-- z = at - cm + player_out - 1
		-- cm = at + player_out - 1 - z
			end
		end
		
		table.insert(nda, e)
	end
end
gm.da = nda

if pressed[KEY_SPACE] then
	if gm.mode == 0 then
		start_game()
	elseif gm.grounded and not gm.realdead and gm.mode > 0 then
		s_jump:Play(false)
		gm.dz = -0.5
		gm.grounded = nil
	end
end

if gm.mode < 0 and cm.z > 100 then
	gm.mode = 0
	dos:set(0,0,"FALL2")
	dos:set(28,0,"KOTM#9/15/12")
	dos:set_centered(0,21,40,"CONTROLS:#\008#\021#\010#\011#SPACE")
	dos:set_centered(0,22,40,"FALL2#AS#FAR#AS#YOU#CAN")
	dos:set_centered(0,23,40,"BUT#DON'T#HURT#YOURSELF")
	gm.dz = 0
end

if gm.mode > 0 then
	dosScore()
end
local svol = gm.dz
if svol > 0 then
	svol = clamp(0,gm.dz-0.2,1)
	s_fall:setVolume(svol*svol)
	s_fall:setPitch(svol)
	if not s_fall:isPlaying() then s_fall:Play(true) end
else
	if s_fall:isPlaying() then s_fall:Stop() end
	s_fall:setVolume(0.0)
end	

pressed = {}
place_player()