-- On Update

-- Read keyboard results
local any = false
local move = a(Vector3(0,0,0))
if down[KEY_LEFT] or down[KEY_a] then
	any = true
	move.x = -moveby
end
if down[KEY_RIGHT] or down[KEY_d] then
	any = true
	move.x = moveby
end
if down[KEY_UP] or down[KEY_w] then
	any = true
	move.z = -moveby
end 
if down[KEY_DOWN] or down[KEY_s] then
	any = true
	move.z = moveby
end
move.y = -moveby

-- Read keyboard results -- JUMP
if down[KEY_SPACE] and onground then -- hover -- debug, remove later
	any = true
	downv = uponjump
	snd_jump:Play(False)
	down[KEY_SPACE] = false
end

-- Wacky block handling --
for i,block in ipairs(moving) do
	local bcur = block:getPosition()
	local offset = math.cos(ticks/math.pi/block.by)/block.as
	bcur.y = bcur.y + offset
	vSetPosition(block,bcur)
	delete(bcur)
	if block.wasground == ticks-1 then
		local cur = p:getPosition()
		cur.y = cur.y + offset
		vSetPosition(p, cur)
		delete(cur)
	end
end

-- Movement handling
function tryoff(off)
	local cur = p:getPosition()
	vSetPosition(p, vAdd(cur, off))
	local k = bridge:collides(p,inset)
	if k then
		vSetPosition(p, cur)
		if bridge:custEntityType(k) == "exit" then
			did_win = true
		end
	end
	delete(cur)
	return k
end

downv = downv + downaccel
if downv < downcap then downv = downcap end
move.y = downv

tryoff(a(Vector3(move.x,0,0)))
tryoff(a(Vector3(0,0,move.z)))

do
	local yfail = tryoff(a(Vector3(0,move.y,0)))
	local new_onground = yfail and move.y < 0
	if (yfail) then
		local dir = 1 if move.y > 0 then dir = -1 end
		local cur = p:getPosition()
		local with = yfail:getPosition()
		local bBox = bridge:bBox(yfail)
		cur.y = with.y + dir*(bBox.y/2+0.5)
		vSetPosition(p, cur)
		delete(cur) delete(with) delete(bBox)
		if yfail.as then yfail.wasground = ticks end
	end
	if new_onground and not onground then
		snd_land:Play(false)
	end
	onground = new_onground
end

mutateVisibleBlock()

do
	local cam = s:getDefaultCamera():getPosition()
	local cur = p:getPosition()
	if cur.z < cam.z - 8 then
		cam.z = cur.z + 8
		s:getDefaultCamera():setPosition(cam.x,cam.y,cam.z)
	end
	if cur.y < -8 then
		bridge:load_room("media/overlay/dead")
	elseif did_win then
		bridge:load_room("media/overlay/win")
	end
	if pat.z - cur.z > nextloopat then
		new_loop()
	end
	delete(cam) delete(cur)
end

if autorelease then
	for i,one in ipairs(autorelease) do
		delete(one)
	end
	autorelease = {}
end