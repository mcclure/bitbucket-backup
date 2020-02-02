-- Music load
au = au or {}

if not au.thump then
	au.thump = Sound("media/toml.wav")
	au.thump:setVolume(0.5)
end

if not au.clack then
	au.clack = Sound("media/snaredrum.wav")
end

au.k = {halfstep = math.pow(2,1/12)}

function pitchFrom(semitone)
	return math.pow(au.k.halfstep, semitone)
end

function newTone(semitone,modlen)
	semitone = pitchFrom(semitone or 0)
	modlen = modlen or 1
	local data = NumberArray()
	local limit = math.floor((3200/semitone) * modlen)-1
	local v
	for i=0,limit do
		v = math.sin(i*math.pi/160 * semitone)/6
		data:push_back(v) -- *((limit-i)/limit/2)
	end

	local s = bridge:soundFromValues(data)
	delete(data)
	s:setVolume(1.0)
	return s
end


function newHiss(per, ceiling)
	per = per or 0.01
	ceiling = ceiling or 0.25
	semitone = pitchFrom(semitone or 0)
	local data = NumberArray()
	local limit = math.floor((3200*16))-1
	local v = 0
	for i=0,limit do
		v = v + per * (math.random()>0.5 and 1 or -1)
		if v < -ceiling then v = -ceiling elseif v > ceiling then v = ceiling end
		data:push_back(v) -- *((limit-i)/limit/2)
	end

	for i=0,limit do
		data:push_back( data:get(limit-1-i) )
	end

	local s = {bridge:soundFromValues(data), bridge:soundFromValues(data)}
	delete(data)
	return s
end

function newStatic(ceiling)
	ceiling = ceiling or 0.25
	semitone = pitchFrom(semitone or 0)
	local data = NumberArray()
	local limit = math.floor((3200*16))-1
	local v
	for i=0,limit do
		v = math.random()*ceiling*2-ceiling
		data:push_back(v) -- *((limit-i)/limit/2)
	end

	local s = {bridge:soundFromValues(data), bridge:soundFromValues(data)}
	delete(data)
	return s
end

if not au.hums then
	au.hums = {}
	local o = 4
	for i,v in ipairs({-36+o,-24,-24+o,-12,-12+o,o,24,24+o}) do
		local key = string.format("hum%d",i)
		local hum = newTone(v)
		au[key] = hum
		table.insert(au.hums, hum)
	end
	au.hum = newTone(0)
end

if not au.hiss then
	au.hiss = newHiss(0.01)
	au.hiss1 = au.hiss[1]
	au.hiss2 = au.hiss[2]
end

if not au.static then
	au.static = newStatic(0.128/4)
	au.static1 = au.static[1]
	au.static2 = au.static[2]
end

if not au.beep then
	au.beep = newTone(48,8)
end

function clearsound()
	for k,v in pairs(au) do
		if type(v) == "table" and v.__delete then delete(v) end
	end
	au = nil
	bridge:fake()
end