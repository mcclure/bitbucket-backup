-- On Update

if pressed[KEY_SPACE] then
	fm.lives = 3
	fm.level = 1
	fm.score = nil
	fm.kills = 0

	bridge:load_room_txt("media/game.txt")
end

pressed = {}