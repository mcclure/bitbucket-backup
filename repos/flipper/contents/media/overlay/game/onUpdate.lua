-- On Update

if down[0] then
	killDos()
	drag = vSub(mouseAt,mouseDownAt)
	cm.xang = cm.xang - drag.x
	cm.yang = clamp(-108,cm.yang + drag.y,0)
	updateCamera()

	delete(mouseDownAt) mouseDownAt = vDup(mouseAt)
end

if down[KEY_RIGHT] then
	mm.move = 1
	mm.zaxis = false
elseif down[KEY_LEFT] then
	mm.move = -1
	mm.zaxis = false
elseif down[KEY_UP] then
	mm.move = -1
	mm.zaxis = true
elseif down[KEY_DOWN] then
	mm.move = 1
	mm.zaxis = true
end
if down[KEY_UP] or down[KEY_SPACE] then
	mm.jump = true
end

updateBoard()
updateMusic()

memory_drain()