-- Game over screen frame update

dos:clear()
for i, v in ipairs(dos_write) do
	if i > 0 and v[1] <= ticks then toggle(v) end
	if v[2] then dos:set(4,v[3],v[4]) end
end

cube:setYaw(ticks/10) cube:setPitch(ticks/10)

if titleDone then
	gm = nil
	bridge:load_room_txt("media/game.txt")
end