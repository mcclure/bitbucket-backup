-- On Update

if not bridge:term_busy() then
	for k,v in pairs(gm.onInput) do
		v:onInput()
	end

	for k,v in pairs(gm.onTick) do
		v:onTick()
	end
end

if gm.pleasereset then
	--audio_reset()
	bridge:rebirth()
elseif pressed[KEY_ESCAPE] then
	bridge:load_room_txt("media/exit.txt")
end


pressed = {} -- Must be in onUpdate if "pressed" is being used