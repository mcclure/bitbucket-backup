-- Music update
if bgmusic then

	if slipat then
		if not lastslip then
			lastslip = 0
		else
			local co = bgmusic:getOffset()
			if co < lastslip or (co-lastslip) > slipat then
				if not slipstart then slipstart = 0 end
				if not slipend then slipend = bgmusic:getSampleLength() end
				local newat = math.random(slipstart, slipend)
				bgmusic:setOffset(newat)
				lastslip = newat
			end
		end
	end

	if playend then
		local co = bgmusic:getOffset()
		if co >= playend then
			bgmusic:setOffset(playstart)
			if lastslip then
				local slipsize = co-lastslip
				lastslip = playstart - slipsize
			else
				lastslip = playstart
			end
		end
	end
	
end


if _DEBUG then
	if bgmusic and down[KEY_F5] then
		print("\tOFFSET:")
		print(bgmusic:getOffset())
		print("\tSAMPLE LENGTH:")
		print(bgmusic:getSampleLength())
		print("\tLASTSLIP:")
		print(lastslip)
	end
	if down[KEY_F6] then
		shadlast.px = shadlast.px * 2
		shadset.px:setNumber(shadlast.px)
		shadlast.py = shadlast.py * 2
		shadset.py:setNumber(shadlast.py)
		print(shadlast.px)
	end
	if down[KEY_F7] then
		shadlast.px = shadlast.px * 0.5
		shadset.px:setNumber(shadlast.px)
		shadlast.py = shadlast.py * 0.5
		shadset.py:setNumber(shadlast.py)
		print(shadlast.py)
	end
end