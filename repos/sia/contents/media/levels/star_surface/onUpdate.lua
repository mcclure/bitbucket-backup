if fm.tickon:get() then
	au.tick:setVolume(1.0)
	au.tick:Play(false)
	
	fm.starAt = fm.starAt + 1
	tickRotate(fm.starAt)
end