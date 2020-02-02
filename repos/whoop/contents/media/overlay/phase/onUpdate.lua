-- On Update

for i,v in ipairs(gm.w) do
	v:tick()
end

if (pressed[0] or pressed[1]) and mouseDownAt then
	local x,y = deClick(mouseDownAt.x,mouseDownAt.y)
	local isback = pressed[1] or down[KEY_LSHIFT] or down[KEY_RSHIFT]
	local dir = isback and -1 or 1
	if x and y then 
		if x > 19 then x = 39 - x end
		if y > 11 then y = 23 - y end
		local at = math.min(x,y)
		local which = gm.w[at+1]
		
		if down[KEY_LCTRL] or down[KEY_RCTRL] then
			which.mute = not which.mute
			which.tone:setVolume(which.mute and 0 or 1)
			which:clr(which.dy~=0 and not which.mute)
		elseif not which.mute then
			which.istep = which.istep + dir
			delete(which.tone)
			which.tone = newTone(which.istep)
			which.tone:Play(true)
			which.left = km.flast
			which.ldir = -dir
		end
	end
end

if pressed[KEY_RIGHT] then km.on = km.on - (km.on > 1 and 1 or 0) end
if pressed[KEY_LEFT] then km.on = km.on + 1 end

if (not _DEBUG) and pressed[KEY_r] then nextroom("phase") end

local didnumber = false

for k,v in pairs(km.remap) do
	if pressed[k] then
		for k2,v2 in ipairs(gm.w) do
			v2.istep = v(k2-1)
			delete(v2.tone)
			v2.tone = newTone(v2.istep)
			v2.tone:Play(true)
		end
		didnumber = true
		break	
	end
end

if pressed[KEY_b] or didnumber then
	for k,v in ipairs(gm.w) do
		local i = k - 1
		v.x = i v.y = i v.dx = 1 v.dy = 0 v.p = 1
	end
end

if pressed[KEY_ESCAPE] then
	bridge:load_room_txt("media/init.txt")
end

pressed = {}