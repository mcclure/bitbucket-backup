-- SOUNDS

have_audio = false -- For someday control of when sounds are reloaded
if not have_audio then
	have_audio = true
	
	-- Jump
	if s_jump then delete(s_jump) s_jump = nil end
	s_jump = Sound("media/doorslam.ogg")

	-- Land
	if s_land then delete(s_land) s_land = nil end
	s_land = Sound("media/jdrums23.ogg")
	
	if s_louch then delete(s_louch) s_louch = nil end
	s_louch = Sound("media/ouch.ogg")
	
	-- Generate shoot sound
	if s_tart then delete(s_tart) s_tart = nil end
	do
		local data = a(NumberArray())
		local limit = (512*32)-1
		local w = 20
		local v
		for i=0,limit do
			v = ((math.floor(i/w)%2)==0) and 0.25 or -0.25
			data:push_back(v*((limit-i)/limit))
		end
		s_tart = bridge:soundFromValues(data)
	end
	
	-- Generate kill sound
	if s_fall then delete(s_fall) s_fall = nil end
	do
		local data = a(NumberArray())
		local limit = (512*64)-1
		local w = 20
		local v
		for i=0,limit do
			v = (math.random() > 0.5) and 0.25 or -0.25
			--if i % 20 == 0 then w = w + 1 end
			data:push_back(v)
		end
		s_fall = bridge:soundFromValues(data)
		s_fall:setVolume(0.0)
	end
	
	if s_ow then delete(s_ow) s_ow = nil end
	do
		local data = a(NumberArray())
		local limit = (512*32)-1
		local w = 20
		local v
		for i=0,limit do
			v = ((math.floor(i/w)%2)==0) and 0.25 or -0.25
			if i % 80 == 0 then w = w + 1 end
			data:push_back(v*((limit-i)/limit))
		end
		s_ow = bridge:soundFromValues(data)
	end
end