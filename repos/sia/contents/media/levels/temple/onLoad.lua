addScreen("temple", km.roomObj)

pull(gm, {bigat = 2, litat = 6,})
--pull(km, {})

local clock = id("clock")
if clock then clock = ScreenEntity(clock) end
local clockat = a(clock:getPosition())
local zero = a(Vector3(0,0,0))
local up = vDup(zero)

up.y = clock:getHeight()/2 - 2
gm.bighand = ScreenLine(zero, up)
vSetPosition(gm.bighand, clockat)
gm.bighand:setColor(0,0,0,1)
gm.bighand:setLineWidth(4)
screens.temple:addChild(gm.bighand)

up.y = clock:getHeight()/8*3
gm.lithand = ScreenLine(zero, up)
vSetPosition(gm.lithand, clockat)
gm.lithand:setColor(0,0,0,1)
gm.lithand:setLineWidth(4)
screens.temple:addChild(gm.lithand)

gm.lithand:setRotation(gm.litat / km.handticks * 360 + 180)
gm.bighand:setRotation(gm.bigat / km.handticks * 360 + 180)

plant(gm.lithand)
plant(gm.bighand)

gm.angel_discussion = Dialogue({
	gstart = "basic",	
	blank={"angel1","angel2","angel3"},
	create="temple_clerk",
	tree = {start = {say={"What are you. You are. You are here.\nWhat are you here."},options={ {say="Who are you?",go="resp1"}, {say="What's behind the door?",go="resp2"} }},
		resp1 = {say={"I accumulate. I fold. I spindle. I\nstaple. I do all these things and more. My powers are limitless.", "Those who stand in the way of my duties shall stand no chance. Those who resist will be filed with the rest."},},
		resp2 = {say={"Nothing is through the door. There is noworld outside of this room."}},},
})