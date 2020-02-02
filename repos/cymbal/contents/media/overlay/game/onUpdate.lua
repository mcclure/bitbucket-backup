-- On Update

local now = ticks
if not gm.over and now - gm.lastTick > km.tempo then
	au[math.random(2)]:Play(false)
	gm.lastTick = now

	for _,v in ipairs(gm.clickers) do
		if v.onClick then v:onClick() end
	end
	
	local overgrowth = g.max - (km.boardh - km.growbuf) - 1
	if overgrowth > 0 then
		for _,v in ipairs(gm.clickers) do v:scroll(overgrowth) end
		gfx:pxscroll(-overgrowth*km.tilesize, 0)
	end
	if p1.max < 1 or p2.max < 1 then
		gm.over = true
		if false or (p1.max < 1 and p2.max < 1) then
			dos:set(0,22,"IT'S A DRAW!")
			dos:set(0,23,"YOU ARE BOTH TERRIBLE!")
		elseif p1.max < 1 then
			dos:set(0,22,"PLAYER 2 YOU WIN!")
			dos:set(0,23,"YOU ARE AMAZING!")
		else
			dos:set(0,22,"PLAYER 1 YOU WIN!")
			dos:set(0,23,"YOU ARE AMAZING!")
		end
		dos:set(29, 22,"PLAY AGAIN?")
		dos:set(37, 23, "Y/N")
	end
elseif gm.over then
	if pressed[KEY_y] then
		bridge:rebirth()
	elseif pressed[KEY_n] then
		bridge:Quit()
	end
end