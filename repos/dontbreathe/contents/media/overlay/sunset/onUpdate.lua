--if ticks == 2 then
--	removethese({"sun1","sun2","sun3","sun4","label"})
--end

-- load_next_if("sensor", "sun_in")


if down[KEY_LEFT] then
	thresher(shader.thresh:get() - km.tglide)
	updateSound()
elseif down[KEY_RIGHT] then 
	thresher(shader.thresh:get() + km.tglide)
	updateSound()
end

if down[KEY_UP] then
	sizer(gm.poff - 1)
	updateSound()
elseif down[KEY_DOWN] then 
	sizer(gm.poff + 1)
	updateSound()
end
	
if pressed[KEY_ESCAPE] then bridge:load_room_txt("media/init.txt") end

pressed = {}
