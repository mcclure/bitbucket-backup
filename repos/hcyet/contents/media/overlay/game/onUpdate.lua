-- On Update

if not gm.dead then
	if not tableTrue(down) then
		local last = nil
		while 1 do
			local n = listeners.input:pop()
			if not n then break end
			last = n
		end
		if last then
			local letter = string.lower(last[2])
			if isin(letter, km.alphabet) then
				local check = gm.current .. letter
				if gm.model:hits(check)>0 then
					gm.current = check
					resetCurrent()
				else
					dying("invalid play")
				end
			end
		end
	end
end

pressed = {} -- Must be in onUpdate if "pressed" is being used