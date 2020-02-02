-- Input handler -- On Update

if down[KEY_ESCAPE] then
	gm.controller:file_save()
	bridge:Quit()
end

pressed = {}

if want_rebirth then
	want_rebirth = false
	bridge:rebirth()
end