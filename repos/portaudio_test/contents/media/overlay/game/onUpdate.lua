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
if down[KEY_r] and not _DEBUG then
	bridge:rebirth(false)
end

if gm.d and _DEBUG then
	player.x = player.x + want.x
	player.y = player.y + want.y
else
	player:try(want.x,"x")
	player:try(want.y,"y")
end

--local below = player.y + player.h - cm.yo - km.sfloor
--if below > 0 then
--	cm.yo = cm.yo + below
--	cm.yo = clamp(0, cm.yo, map.fore_g.h - gfx_floor)
--	drawbg(0,-cm.yo)
--end

function crush(v, v2, lim)
	return clamp(0, (v - 3) / (lim - 6 - v2), 1)
end
local toney, tonex = crush(player.x, player.w, gfx_width), crush(player.y, player.h, gfx_floor)
local primes = {math.exp(1),math.pi*2,math.sqrt(13)*3}
local anyflat = 0

gm.scratch:pxcopy(gm.graph, -1, 0, 0, 0)
gm.graph:pxcopy(gm.scratch)
for _i=1,gm.sound:toneCount() do
	local i = _i-1
	local BDT = 3
	local flat = gm.sound:ltone(4,i)>0
	local dt = math.sin(flat and gm.sound:ltone(5,i) or gm.sound:ltone(3,i))
	local x = gfx_width-1
	local y = (1-dt)/2 * gfx_floor
	
	if flat then anyflat = anyflat + 1 end
	
	gm.graph:pxcopy_invert(gm.graph, x, y, x, y, 1, 1)
	
	gm.sound:setTone(0, i, math.pow(primes[_i],tonex))
	gm.sound:setTone(1, i, toney/4410)
end

drawbg()

if toney > 0 then
	local off = anyflat>1 and (tonex * -10) or 0
	dos.g:pxcopy_noisy(toney/8, dos.g, 0, 40, 0, 40, -1, 1)
	dos.g:pxcopy_noisy(toney/4, dos.g, 0, 165+off, 0, 165+off, -1, 5)
end

player:enter()

if pressed[KEY_ESCAPE] then
	if nextroom then nextroom("attract")
	else bridge:Quit() end
end

pressed = {}