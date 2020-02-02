if retain then
	for i,one in ipairs(retain) do
		delete(one)
	end
	retain = nil
end