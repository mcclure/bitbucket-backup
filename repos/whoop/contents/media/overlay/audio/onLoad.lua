-- Audio setup on load

if not au then au = {} end

au.k = {halfstep = math.pow(2,1/12)}

function pitchFrom(semitone)
	return math.pow(au.k.halfstep, semitone)
end

function newTone(semitone)
	semitone = pitchFrom(semitone or 0)
	local data = NumberArray()
	local limit = math.floor((3200/semitone))-1
	local v
	for i=0,limit do
		v = math.sin(i*math.pi/160 * semitone)/16
		data:push_back(v) -- *((limit-i)/limit/2)
	end

	local s = bridge:soundFromValues(data)
	delete(data)
	s:setVolume(1.0)
	return s
end

-- WAIT WHY AM I PUTTING UTILITY FUNCTIONS IN AUDIO/

function nextroom(which)
	bridge:load_room(string.format("media/overlay/startup\nmedia/overlay/audio\nmedia/overlay/%s\nmedia/title.svg\nmedia/overlay/shutdown",which))
end

function dosKm()
	local factor = dos:factor()
	km.celly = factor*8
	km.cellx = factor*7
	km.dosfromy = (surface_height - km.celly*24)/2
	km.dosfromx = (surface_width - km.cellx*40)/2
end

function deClick(x,y)
	local xcell, ycell = 
	math.floor((x-km.dosfromx)/km.cellx),
	math.floor((y-km.dosfromy)/km.celly)
	if xcell >= 0 and ycell >= 0 and xcell < 40 and ycell < 24 then
		return xcell,ycell
	else
		return nil,nil
	end
end

if _DEBUG then 
	function audio_reset()
		for k,v in pairs(au) do
			if v.__delete then delete(v) end
		end
		au = {}
		bridge:rebirth()
	end
end