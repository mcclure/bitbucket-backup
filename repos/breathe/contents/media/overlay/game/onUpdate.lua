-- On Update

local anyMove = false
local move = a(Vector3(0,0,0))
if down[KEY_LEFT] then -- or down[KEY_a] then
	anyMove = true
	move.x = -moveby
end
if down[KEY_RIGHT] then -- or down[KEY_d] then
	anyMove = true
	move.x = moveby
end
if down[KEY_UP] then -- or down[KEY_w] then
	anyMove = true
	move.y = -moveby
end 
if down[KEY_DOWN] then -- or down[KEY_s] then
	anyMove = true
	move.y = moveby
end

function vOffsetPosition(e,v) -- Since setPosition(vector) isn't exported
	local cv = e:getPosition()
	e:setPosition(cv.x+v.x,cv.y+v.y,cv.z+v.z)
end
function vOffsetRotation(e,d)
	local a = e:getRotation()
	e:setRotation(a + d)
end

function clamp(x,a,b)
	return math.min(math.max(x,a),b)
end

if player then
	local e = player
	local v = move
	local cv = e:getPosition()
	if anyMove then
		e:setPosition(clamp(cv.x+v.x,0,surface_width),clamp(cv.y+v.y,0,surface_height),cv.z+v.z)
		vOffsetRotation(player,spinby)
	end
	collide = screen():getEntityAt(cv.x+playerinfo[2]/2,cv.y+playerinfo[3]/2)
end

if ticks == 2 then
	screen():removeChild(id("bg"))
end