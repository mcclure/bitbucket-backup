-- On Update

if loadmessage then
	bridge:loadPak(loadmessage)
elseif listener:peek() then
	local ct = listener:pop()
	local ck, cc = unpack(ct)
	
	if ck >= 48 and ck <= 57 then -- 0-9
		local num = ck-48
		if num >= 0 and num <= #menu then
			local choice = num+1 -- Notice we shift up one to get the hidden "0"
			local what = menu[choice][1]
			if menu[choice].slowload then
				loadmessage = what
				dos:set(menu[choice].left,menu[choice].top,"     Loading     ", true)
			else
				bridge:loadPak(what)
			end
		end
	end
end

pressed = {}