if fm.tickon:get() then
	au.tick:setVolume(0.375)
	au.tick:Play(false)
	
	fm.starAt = fm.starAt + 1
	tickRotate(fm.starAt)
end