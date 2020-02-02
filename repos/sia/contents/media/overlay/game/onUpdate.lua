-- Game on update

while not act:empty() do
	action = act:peek()
	if gm.expedite then action.wait = 0 gm.expedite = false end
	if action.wait and action.wait > 0 then
		action.wait = action.wait - 1
		break
	end
	act:pop()
	why = action[1]
--	print({ticks, why, action.wait, action.target and bridge:room_name(action.target)})
	
	if why == "enable" then
		bridge:setVisible(action.target, true)
	elseif why == "disable" then
		bridge:setVisible(action.target, false)
	elseif why == "go" then
		local fullname = string.format("media/overlay/startup\nmedia/overlay/pregame\nmedia/overlay/audio\nmedia/overlay/glitter\nmedia/levels/%s\nmedia/overlay/game\nmedia/title.svg\nmedia/overlay/shutdown", action.to)
		bridge:load_room(fullname)
	elseif why == "di" then
		gm.dialog:add(action.a)
	elseif why == "do" then
		action.this(action)
	end
end

if _DEBUG and dx then
	dx()
	dx = nil
end

if gm.dialog then
	gm.dialog:tick()
end

if wantfps and ticks%60 == 0 then
	print(string.format("%0.1f FPS", Services.Core:getFPS()))
end