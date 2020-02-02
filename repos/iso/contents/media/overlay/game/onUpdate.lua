-- On Update

if not bridge:term_busy() then
	for k,v in pairs(gm.onInput) do
		v:onInput()
	end

	for k,v in pairs(gm.onTick) do
		v:onTick()
	end
end

if gm.text_timeout and ticks-gm.text_timeout > km.text_timeout_on then
	clear_dialog_text()
end

if loadnext then
	if _DEBUG and fm.lastLoaded and not fm.block_autosave then savelevel(fm.lastLoaded) end
	loadlevel(loadnext, loadnextanchor)
	loadnext = nil
end

if loadroomnext then
	bridge:load_room_txt(loadroomnext)
end

pressed = {}