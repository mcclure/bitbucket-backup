function tickRotate(at)
	ScreenEntity(id("ticker")):setRotation(at / km.handticks * 360)
end

au.rainbow:setVolume(0.05)

addScreen("rroad_3", km.roomObj)

tickRotate(fm.starAt)