-- Attract mode -- On Update

if pressed[KEY_SPACE] then
	bridge:load_room_txt("media/game.txt")

elseif pressed[KEY_p] then
	bridge:load_room_txt("media/practice.txt")

elseif pressed[KEY_ESCAPE] then
	bridge:Quit()
	
end

pressed = {} -- Must be in onUpdate if "pressed" is being used