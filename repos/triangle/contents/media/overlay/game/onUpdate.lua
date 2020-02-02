-- On Update

local movex, movey = nil,nil

if down[KEY_LEFT] then
	movex = -1
elseif down[KEY_RIGHT] then
	movex = 1
end

if down[KEY_UP] then
	movey = -1
elseif down[KEY_DOWN] then
	movey = 1
end

if pressed[KEY_l] then
	shaders[2].brightness:set(1.03)
elseif pressed[KEY_k] then
	shaders[2].brightness:set(1.02)
end

if movex or movey then -- MOVEMENT OFF
	gm.physics:applyForce(gm.p, (movex or 0)*km.movef, (movey or 0)*km.movef)
	if not gm.moving then
--		gm.sndcontroller:iparam("noiserepeats", 9)
		gm.moving = true
	end
	gm.color:setColorHSV(gm.thue, 1.0,1.0)
	gm.thue = (gm.thue + 1) % 360
	bridge:setColorObj(gm.p, gm.color)
else
	if gm.moving then -- MOVEMENT ON
--		gm.sndcontroller:iparam("noiserepeats", 67)
		gm.moving = false
	end
end

pressed = {} -- Must be in onUpdate if "pressed" is being used