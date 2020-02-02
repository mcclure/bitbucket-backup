if not au then

au = {}

au.k = {halfstep = math.pow(2,1/12)}

function pitchFrom(semitone)
	return math.pow(au.k.halfstep, semitone)
end

function newTone(semitone, lenmod)
	semitone = pitchFrom(semitone or 0)
	local data = NumberArray()
	local limit = math.floor(44100 / 4 * lenmod)
	local v
	for i=0,limit do
		v = math.sin(i*math.pi/160 * semitone)/16
		v = v * math.sin(i*math.pi/limit)
		data:push_back(v) -- *((limit-i)/limit/2)
	end

	local s = bridge:soundFromValues(data)
	delete(data)
	s:setVolume(1.0)
	return s
end

function newSqTone(semitone, lenmod)
	semitone = pitchFrom(semitone or 0)
	local data = NumberArray()
	local limit = math.floor(44100 / 4 * lenmod)
	local v
	for i=0,limit do
		v = math.sin(i*math.pi/160 * semitone)
		v = v > 0 and 1 or -1
		v = v/16
		v = v * math.sin(i*math.pi/limit)
		data:push_back(v) -- *((limit-i)/limit/2)
	end

	local s = bridge:soundFromValues(data)
	delete(data)
	s:setVolume(1.0)
	return s
end

function silence()
	local data = NumberArray()
	local limit = math.floor(256)-1
	for i=0,limit do
		lastv = v
	end

	local s = bridge:soundFromValues(data)
	delete(data)
	s:setVolume(0.0)
	return s
end


local aud = {0,7,12,19,24}

au.tones = {}
au.squares = {}

for i,v in ipairs(aud) do
	local offset = 1+math.random()
	local snd = nil
	
	snd = newSqTone(v-12, math.pow(math.pi, offset))
	snd:Play(true)
	table.insert(au.squares, snd)
	table.insert(au, snd)
	
	snd = newTone(v, math.pow(math.pi, offset))
	snd:Play(true)
	snd:setVolume(0)
	table.insert(au.tones, snd)
	table.insert(au, snd)
end

end

if 1 or _DEBUG then 
	function audio_reset(nobirth)
		for k,v in pairs(au) do
			if v.__delete then delete(v) end
		end
		au = nil
		if not nobirth then bridge:rebirth() end
	end
end