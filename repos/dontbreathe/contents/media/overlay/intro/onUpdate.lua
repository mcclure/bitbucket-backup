--if ticks == 2 then
--	removethese({"sun1","sun2","sun3","sun4","label"})
--end

-- load_next_if("sensor", "sun_in")

if special() then

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
	
end

if listener:peek() then
	local ct = listener:pop()
	local ck, cc = unpack(ct)
	
	if ck >= 48 and ck <= 57 then -- 0-9
		km.maxobj = ck-48 - 1
		au.beep:Play(false)
	elseif ck >= 32 and ck <= 126 then
		assertEntry()
		updateEntry(gm.entrytext .. cc)
		au.clack:Play(false)
	elseif ck == 127 or ck == 8 then
		if gm.entrytext then
			local count = #gm.entrytext
			if count > 0 then
				assertEntry()
				updateEntry(gm.entrytext:sub(1,count-1))
				au.clack:Play(false)
			end
		end
	end
end


local clickat = ticks%km.clickticks
local floatcount = nil
function getFloatCount()
	if not floatcount then floatcount = tableCount(gm.floaters) end
	return floatcount
end
function everythingGone()
	return (km.maxobj<0 and getFloatCount()==0)
end

if ticks>0 then
	if clickat==0 then
		if not everythingGone() then
			if getFloatCount() > km.maxobj then
				local fids = idsfrom(gm.floaters)
				local targetid = fids[math.random(#fids)]
				local target = gm.floaters[targetid]
				screen():removeChild(target)
				gm.floaters[targetid] = nil
				delete(target);
			else
				spawn()
			end
		end
		clearEntry()
	end
	
	local thumppitch = km.thumpat[clickat]
	if thumppitch and not everythingGone() then
		au.thump:setPitch(thumppitch)
		au.thump:Play(false)
	end
end

updateSoundInstant()

if pressed[KEY_ESCAPE] then bridge:load_room_txt("media/init.txt") end

pressed = {}