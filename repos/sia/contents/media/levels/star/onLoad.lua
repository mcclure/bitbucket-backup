function tickRotate(at)
	ScreenEntity(id("ticker")):setRotation(at / km.handticks * 360)
end

if au.rainbow:isPlaying() then
	au.rainbow:Stop()
end

addScreen("star", km.roomObj)

tickRotate(fm.starAt)