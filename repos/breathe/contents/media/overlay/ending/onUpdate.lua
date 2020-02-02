if ticks == 2 then
	removethese({"sun1","sun2","sun3","sun4","label"})
end

do
	if collide then
		local cname = bridge:room_name(collide)
		if cname == "sensor" then
			wonat = ticks
		end
	end
end

local score = ticks - won
if score == 5*60 then
	screen():removeChild(player)
	player = nil
	origpx = shadlast.px
	origpy = shadlast.py
end
if score > 10*60 then
	shadlast.px = shadlast.px + origpx/2
	shadset.px:setNumber(shadlast.px)
	shadlast.py = shadlast.py + origpy/2
	shadset.py:setNumber(shadlast.py)
end
if score > 15*60 then
	bgmusic:Stop()
end

memory_drain()