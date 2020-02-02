if ticks == 2 then
	screen():removeChild(pic)
end

if not load_next_if("sensor", "guns") then
	if player:getPosition().x > surface_width-100 then
		loadnext("guns")
	end
end


memory_drain()