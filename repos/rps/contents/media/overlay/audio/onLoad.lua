-- Audio utility -- On Load

-- Basic sounds

if not au then
	au = {}
	au[1] = Sound("media/sound/jdrums23.wav")
	au[2] = Sound("media/sound/snaredrum.wav")
	au[3] = Sound("media/sound/clap.wav")
	
	au.k = {halfstep = math.pow(2,1/12)}
end

-- Make sounds

function pitchFrom(semitone)
	return math.pow(au.k.halfstep, semitone)
end

function newTone(semitone)
	semitone = pitchFrom(semitone or 0)
	local data = NumberArray()
	local limit = math.floor(3200*4)-1
	local v
	for i=0,limit do
		v = math.sin(i*math.pi/80 * semitone)/16
		data:push_back(v *((limit-i)/limit/2))
	end

	local s = bridge:soundFromValues(data)
	delete(data)
	s:setVolume(1.0)
	return s
end

function newBeep(semitone,decay)
	semitone = pitchFrom(semitone or 0)
	local data = NumberArray()
	local limit = math.floor(3200/2*(decay or 1))-1
	local v
	for i=0,limit do
		v = math.sin(i*math.pi/80 * semitone)/16*(decay or 1)
		data:push_back(v)-- *((limit-i)/limit/2))
	end

	local s = bridge:soundFromValues(data)
	delete(data)
	s:setVolume(1.0)
	return s
end

function newSquare(semitone)
	semitone = pitchFrom(semitone or 0)
	local data = NumberArray()
	local limit = math.floor(3200*1.25)-1
	local v
	for i=0,limit do
		v = ( math.sin(i*math.pi/320 * semitone) > 0 and 1 or -1 )/12
		data:push_back(v *((limit-i)/limit/2))
	end

	local s = bridge:soundFromValues(data)
	delete(data)
	s:setVolume(1.0)
	return s
end

function newFall(semitone)
	semitone = pitchFrom(semitone or 0)
	local data = NumberArray()
	local limit = math.floor(3200*3)-1
	local v
	for i=0,limit do
		v = ( math.sin(i*math.pi/320 * semitone / (       math.pow(2, math.max(i-limit/10 ,0)/limit)       ) ) > 0 and 1 or -1 )/6
		data:push_back(v *((limit-i)/limit/2))
	end

	local s = bridge:soundFromValues(data)
	delete(data)
	s:setVolume(1.0)
	return s
end

-- Handle au table

function audio_register(sound, name)
	if name then
		if au[name] and au[name].__delete then delete(au[name]) end
		au[name] = sound
	else table.insert(au, sound) end
	return sound
end

if _DEBUG then
	function audio_reset(nobirth)
		for k,v in pairs(au) do if v.__delete then delete(v) end end
		au = nil if not nobirth then bridge:rebirth() end
	end
end
