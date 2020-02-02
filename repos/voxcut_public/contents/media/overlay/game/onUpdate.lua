-- On Update

local place = ticks/(onepass*60)
local count = 8 - math.floor(place)
if count > 0 then
	dos:set(38,0,string.format("%2d",count))
else
	for i=1,5 do
	local x,y = math.random(0,279),math.random(0,191)
		dos.g:pxset(x,y,math.random(2)==1)
	end
	if not started then
		dos:set(38,0,"  ")
		humEffect2:Play(true)
		started = true
	end
	humEffect:setVolume(1-((place-8) / 32)%1)
end
dos.g:broken_pxcopy(dos.g, 0, gfx_height/4, 0,ticks%(gfx_height/2)+1, gfx_width/2, gfx_height/2)

if pressed[KEY_ESCAPE] then
	bridge:Quit()
end

pressed = {}