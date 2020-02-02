if fm.tickon:get() then
	gm.bigat = gm.bigat + 1
	if gm.bigat >= km.handticks then
		gm.bigat = 0
		gm.litat = gm.litat + 1
		if gm.litat >= km.handticks then
			gm.litat = 0
		end
		gm.lithand:setRotation(gm.litat / km.handticks * 360 + 180)
	end
	gm.bighand:setRotation(gm.bigat / km.handticks * 360 + 180)
	au.tick:Play(false)
end