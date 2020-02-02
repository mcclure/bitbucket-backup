-- On Update

if ticks > 60*3 then
	stopall()
	fm.level = fm.level + 1
	fm.kills = 0
	bridge:load_room_txt("media/game.txt")
end

pressed = {}