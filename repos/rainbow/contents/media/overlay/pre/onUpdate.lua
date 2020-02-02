-- On Update

if pressed[KEY_ESCAPE] then
	bridge:Quit()
elseif pressed[KEY_SPACE] then
	bridge:load_room_txt("media/game.txt")
end

pressed = {}