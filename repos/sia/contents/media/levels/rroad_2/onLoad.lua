if not fm.starAt then fm.starAt = 0 end

function tickRotate(at)
	ScreenEntity(id("ticker")):setRotation(at / km.handticks * 360)
end
tickRotate(fm.starAt)

au.rainbow:setVolume(0.25)

addScreen("rroad_2", km.roomObj)
gm.ray = id("ray")
tickRotate(fm.starAt)

function rayColor()
	if fm.raytime:get() then
		if fm.raytick%6 > 1 then
			fm.rayrot = fm.rayrot + 1
			local i,v = 2,gm.ray
			local wantcolor = fm.raycolor[ (i + fm.rayrot) % 5 + 1 ]
			bridge:setColorObj(v, wantcolor)
		end
		fm.raytick = fm.raytick + 1
	end
end
rayColor()