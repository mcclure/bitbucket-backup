-- On Update

--xo:setNumber(math.sin(ticks/90))

if mode == 0 and 0 < typed.count then
	local t = typed:pop()
	
	if t == "\r" then -- Newline
		dos:set(tx,ty," ")
		printed:push("\n\n")
		handle(entry)
		entry = ""
	elseif t == "\127" then -- delete -- subvert printed() system
		if #entry>0 then
			dos:set(tx,ty," ")
			tx = tx - 1
			if tx < 0 then ty = ty - 1 tx = 39 end
			dos:set(tx,ty," ") -- overkill?
			entry = entry:sub(0,-2)
			cursor_reset = ticks
		end
	else
		entry = entry .. t
		printed:push(t)
		cursor_reset = ticks
	end
end

local grains = 18 -- print time
while 0 < printed.count and 0 < grains do
	local p = printed:peek()
	
	if "table" == type( p ) then
		if p.mode then
			mode = p.mode
		end
		printed:pop()
	else
		if #p <= grains then
			printed:pop()
		else
			printed[printed.low] = string.sub(p,grains+1)
			p = string.sub(p,1,grains)
		end
		
		splat(p)
		
		grains = grains - #p
	end
end

if mode == 0 then
	local on = " "
	if floor((ticks - cursor_reset)/20)%2 == 0 then
		on = "\127"
	end
	dos:set(tx, ty, on)
end

memory_drain()