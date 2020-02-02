-- On Update

if pressed[KEY_y] then
	bridge:load_room_txt("media/game.txt")
elseif pressed[KEY_n] then
	bridge:Quit()
end