-- On Update

for i,e in ipairs(gm.heads) do
	e.i = e.i + 1
	if e.i >= km.blockrollover then
		if e.h then e.bx = e.bx + e.h else e.by = e.by + 1 end -- Move "b" cursor left, right or up
		local relativey = e.by - gm.bscroll
		e.lasth = e.h
		if relativey < km.swervedanger then
			e.h = nil
		elseif relativey > km.block_height - km.swervedanger then
			e.h = (e.bx < km.block_width/2) and 1 or -1
		elseif (not gm.tails:get(e.bx,e.by)) and math.random(4) == 1 then
			e.h = not e.h
			if e.h then
				if e.bx < km.swervedanger then e.h = 1
				elseif e.bx > km.block_width - km.swervedanger then e.h = -1
				elseif math.random(2) == 1 then e.h = 1 else e.h = -1 end
			end
		end
		
		stamp(e) -- Leave a trail
		e.i = 0
	end
	
	local off = e.i*km.blockspeed
	blockPosition(e, e.bx, e.by, e.h and off*e.h or 0, e.h and 0 or off)
end

vScroll(1)
soundFix()

if pressed[KEY_ESCAPE] then
	bridge:Quit()
end

pressed = {}