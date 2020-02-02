-- Game over screen frame update

dos:clear()
for i, v in ipairs(dos_write) do
	if ticks>v[1] then dos:set(4,v[2],v[3]) end
end

if titleDone then
	gm = nil
	bridge:load_room_txt("media/init.txt")
end