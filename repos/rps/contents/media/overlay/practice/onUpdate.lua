-- On Update

if pressed[KEY_ESCAPE] then
	bridge:load_room_txt("media/init.txt")

elseif gm.need_restart then
	bridge:rebirth()
end