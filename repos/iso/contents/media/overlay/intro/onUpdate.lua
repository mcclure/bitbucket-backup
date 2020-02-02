local k = listeners.basic:pop()
if k and k[2] and #k[2]>0 then bridge:load_room_txt("media/game.txt") end

pressed = {}
