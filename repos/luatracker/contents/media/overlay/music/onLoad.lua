-- Audio setup on load

if not au then au = {} end

-- Generate "crash" sound
if not au.tone then
	local data = a(NumberArray())
	local limit = (3200)-1
	local v
	for i=0,limit do
		v = ((math.floor(i/320)%2)==0) and 0.25 or -0.25
		data:push_back(v) -- *((limit-i)/limit/2)
	end
	au.tone = bridge:soundFromValues(data)
end
au.tone:setVolume(1.0)

if _DEBUG then 
	function audio_reset()
		for k,v in pairs(au) do
			delete(v)
		end
		au = {}
		bridge:rebirth()
	end
end