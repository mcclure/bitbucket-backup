if ticks == 2 then
	for i,p in ipairs(removal) do
		screen():removeChild(p)
	end
end

if player:getPosition().x > surface_width-100 then
	loadnext("bigwall")
end

memory_drain()