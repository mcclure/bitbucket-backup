-- On Update

if not bridge:term_busy() then
	for k,v in pairs(gm.monster) do
		v:tick()
	end
	
	for k,v in pairs(gm.ent) do
		v:tick()
	end

	if not (cm.debugLook) then
	else
		if pressed[KEY_LEFT] then
			gm.p.x = gm.p.x - 1
		elseif pressed[KEY_RIGHT] then
			gm.p.x = gm.p.x + 1
		elseif pressed[KEY_UP] then
			gm.p.y = gm.p.y - 1
		elseif pressed[KEY_DOWN] then
			gm.p.y = gm.p.y + 1
		end
	end
	if gm.p.dead and pressed[KEY_SPACE] then
		if fm.lives > 0 then
			bridge:rebirth()
		else
			bridge:load_room_txt("media/init.txt")
		end
		pressed = {} return
	end

	if _DEBUG then
		if pressed[KEY_i] then
			cm.debugLook = not cm.debugLook
		end
	end
end

updateCamera()

pressed = {}