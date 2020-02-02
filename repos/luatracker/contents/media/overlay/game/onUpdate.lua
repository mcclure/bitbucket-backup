-- On Update

player:exit()

want = {x=0,y=0}

if down[KEY_LEFT] then
	want.x = want.x - km.dxy
elseif down[KEY_RIGHT] then
	want.x = want.x + km.dxy
end
if down[KEY_UP] then
	want.y = want.y - km.dxy
elseif down[KEY_DOWN] then
	want.y = want.y + km.dxy
end

if gm.d and _DEBUG then
	player.x = player.x + want.x
	player.y = player.y + want.y
else
	player:try(want.x,"x")
	player:try(want.y,"y")
end

local below = player.y + player.h - cm.yo - km.sfloor
if below > 0 then
	cm.yo = cm.yo + below
	cm.yo = clamp(0, cm.yo, map.fore_g.h - gfx_floor)
	drawbg(0,-cm.yo)
end

player:enter()

pressed = {}