-- On Update
if halted then return end

while not act:empty() do
	action = act:peek()
	if gm.expedite then action.wait = 0 gm.expedite = false end
	if action.wait and action.wait > 0 then
		action.wait = action.wait - 1
		break
	end
	act:pop()
--	why = action[1]
--	print({ticks, why, action.wait, action.target and bridge:room_name(action.target)})
	
	if action.this then
		action.this(action)
	end
	if halted then break end
end

if pressed then pressed = {} end