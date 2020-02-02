local k = listeners.basic:pop()

if not gm.keypress and k and k[2] and #k[2]>0 then gm.keypress = ticks end
if gm.keypress and ticks - gm.keypress > 60 then bridge:Quit() end

pressed = {}
