-- Audio setup on load

if not au then au = {} end

-- "Tick" sound
if not au.tick then
	local data = a(NumberArray())
	local limit = (4096)-1
	local v
	for i=0,limit do
		if i % 10 == 0 then
			v = math.random()*2-1
		end
		data:push_back(v*((limit-i)/limit/2))
	end
	au.tick = bridge:soundFromValues(data)
end
au.tick:setVolume(1.0)

-- Rainbow road gibberish
if not au.rainbow then
	local amp = 3
	local data = a(NumberArray())
	local limit = 16384*8
	local x,y,z = 1,math.pi,1
	for i=0,limit do
		local v = math.sin(i / 44100.0 * 2*math.pi * 110 / sqrt(2)) * math.sin(i / 8192 * math.pi)
		v = v * amp
		
		local v2 = math.sin(i*x / 44100.0 * 2*math.pi * 55 / sqrt(2)) * math.sin(i / 16384 * math.pi)
		v2 = v2 * amp
		
		x = (x + 0.25)%y
		
		data:push_back(clamp(-1, v + v2, 1)/2)
	end

	au.rainbow = bridge:soundFromValues(data)
end
au.rainbow:setPitch(1)

-- Low pitched beep for airlock
if not au.lotone then
	local data = a(NumberArray())
	local limit = (3200*10)-1 -- 3200
	local p = 320 -- 40
	local echo = 640
	local echodie = 0.8
	local v
	local q = Queue()
	for i=0,(limit+echo*10) do
		v = 0
		if i < limit then
			v = ((math.floor(i/p)%2)==0) and 1/32 or -1/32
		end
		if q.count >= echo then
			v = v + echodie * q:pop()
		end
		q:push(v)
		if i > echo*2 then
			data:push_back(v) -- *((limit-i)/limit/2)
		end
	end
	au.lotone = bridge:soundFromValues(data)
end

-- High pitched beep for airlock
if not au.hitone then
	local data = a(NumberArray())
	local limit = (3200*2)-1
	local p = 40
	local echo = 640
	local echodie = 0.8
	local v
	local q = Queue()
	for i=0,(limit+echo*10) do
		v = 0
		if i < limit then
			v = ((math.floor(i/p)%2)==0) and 1/32 or -1/32
		end
		if q.count >= echo then
			v = v + echodie * q:pop()
		end
		if i > echo*2 then
			data:push_back(v) -- *((limit-i)/limit/2)
		end
		q:push(v)
	end
	au.hitone = bridge:soundFromValues(data)
end

-- Hiss of gas escaping
if not au.hiss then
	local data = a(NumberArray())
	local limit = (3200*8)-1
	local v
	for i=0,limit do
		v = math.random() / 8
		data:push_back(v) -- *((limit-i)/limit/2)
	end
	au.hiss = bridge:soundFromValues(data)
end

function alone_pitch(thresh, target)
	if not au.alone then return end
	thresh = thresh or 0.1 -- 1/3 is also very interesting
	target = target or 1
	fm.alonep = fm.alonep * 2/3 + target/3
	if math.random() < thresh then
		fm.alonep = fm.alonep * math.random()
	end

	au.alone:setPitch(fm.alonep)
end

if _DEBUG then 
	function audio_reset()
		for k,v in pairs(au) do
			delete(v)
		end
		au = {}
		bridge:rebirth()
	end
end