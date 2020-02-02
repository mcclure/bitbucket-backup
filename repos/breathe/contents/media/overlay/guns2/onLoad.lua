reset_sound(30000)

removal = {}
for i=0,20 do
	local ne = ScreenImage("media/input/wikigun.png")
	ne:setPosition(math.random(0,surface_width), math.random(0,surface_height), 0)
	ne:setPositionMode(POSITION_CENTER)
	screen():addChild(ne)
	ne:setRotation(270)
	table.insert(removal, ne)
end