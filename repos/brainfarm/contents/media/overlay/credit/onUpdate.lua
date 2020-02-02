-- On Update

if pressed[KEY_ESCAPE] then
	bridge:Quit()
else
	if ticks > km.resetat then
		bridge:load_room_txt("media/game.txt")
	end
end

pressed = {}