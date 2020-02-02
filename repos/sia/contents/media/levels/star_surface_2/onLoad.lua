fm.starAt = 1

addScreen("star_surface_bg", km.roomObj, true)
addScreen("star_surface_bg_2", km.roomObj, false, "star_surface_bg")
addScreen("star_surface_2", km.roomObj)

local sse = screens["star_surface_bg"]:getRootEntity()
sse:setPosition(surface_width, surface_height)

local sse2 = screens["star_surface_bg_2"]:getRootEntity()
sse2:setPosition(surface_width, surface_height)

function getV(rot,offset)
	offset = offset or 0
	rot = (rot + 0.25) % 1.0 - 0.25
	return (2 + rot - offset)*90
end

function tickRotate(at)
	local rot = at / km.handticks
	if screens["star_surface_bg"] then sse:setRotation(getV(rot,1)) end
	if screens["star_surface_bg_2"] then sse2:setRotation(getV(rot,0)) end
end

tickRotate(fm.starAt)