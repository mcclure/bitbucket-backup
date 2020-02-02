-- Gameplay On Update

if gm.player then
	local playerv = gm.p:getVelocity(gm.player)
	
	-- Flicker level
	
	if not gm.playerat then
		gm.playerat = ticks
	end
	if not gm.introdone then -- TODO: This sucks
		local playerticks = ticks-gm.playerat
		local pmod = playerticks%km.introflick
		local pat = math.floor(playerticks/km.introflick)
		
		if pmod == 0 then
			if pat == km.introfor then
				gm.introdone = true
				dos:set_centered(0,6,40,"")
			else
				dos:set_centered(0,6,40,string.format("TANK#%d,#SPECIMEN#%d", gm.level.gid,gm.level.sid))
			end
		elseif pmod == km.introflick/2 then
			--dos:set_centered(0,6,40,"")
		end
	end
	
	-- Handle input
	
	if down[KEY_LEFT] then
		gm.p:applyImpulse(gm.player, -1/1, 0)
	end
	if down[KEY_RIGHT] then
		gm.p:applyImpulse(gm.player, 1/1, 0)
	end
	if pressed[KEY_UP] or pressed[KEY_SPACE] then
		if gm.canjump then
			gm.p:applyImpulse(gm.player, 0, -25/1)
			gm.canjump = false
			gm.lastjump = ticks
			s_jump:Play(false)
		end
	end
	
	if playerv.y > 5 then
		gm.canjump = false
	end
	
	if gm.dead then
		if down[KEY_DOWN] then
			bridge:rebirth()
		elseif (not gm.won) and down[KEY_q] then
			force_load = nil
			bridge:rebirth()
		end
	else
		if pressed[KEY_s] then
			levelend(false, nil, "YOU#SURRENDER.")
		end
	end
	
	-- GRAPHICS
	
	local falloff = nil
	if gm.deadat then
		falloff = math.max(0, 1-(ticks-gm.deadat)/(90*4))
	end
	
	for i,v in ipairs(gm.exits) do
		local vary = math.abs(math.cos(ticks/90*math.pi))
		if falloff then vary = vary * falloff end
		local blue = vary * 0.7 + 0.3
		v:setColor(0, blue, blue, 1)
	end
	
	for i,v in ipairs(gm.lava) do
		local vary = math.random() 
		if math.random() < 0.01 then
			vary = 4
		end
		if falloff then vary = vary * falloff end
		local red = vary*0.2+0.3

		v:setColor(red, 0, 0, 1)
	end
	if gm.fatal then
		local red = math.floor(ticks/2)%2==0 and 0.8 or 0.5
		gm.fatal:setColor(red, 0, 0, 1)
	end
end