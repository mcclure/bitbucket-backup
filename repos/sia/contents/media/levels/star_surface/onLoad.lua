function getV(rot,offset)
	offset = offset or 0
	rot = (rot - 0.25) % 1.0 + 0.25
	return (-rot + offset)*surface_height
end

function tickRotate(at)
	local rot = at / km.handticks
	if screens["star_surface_bg"] then screens["star_surface_bg"]:setScreenOffset(0,getV(rot,1)) end
	if screens["star_surface_bg_2"] then screens["star_surface_bg_2"]:setScreenOffset(0,getV(rot)) end
end

fm.starAt = 1

addScreen("star_surface_bg", km.roomObj)
addScreen("star_surface_bg_2", km.roomObj, true, "star_surface_bg")
addScreen("star_surface", km.roomObj)

tickRotate(fm.starAt)