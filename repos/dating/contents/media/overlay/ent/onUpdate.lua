if not bridge:term_busy() then
	for k,v in pairs(gm.onInput) do
		v:onInput()
	end
end

for k,v in pairs(gm.onTick) do
	v:onTick()
end