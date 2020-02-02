-- On Update

local framedone = False
while not framedone and typed.count > 0 do
	local t = typed:pop()
	if game[mode].onKey then
		framedone = game[mode]:onKey(t)
	end
end

if game[mode].onTick then
	game[mode]:onTick()
end

if autorelease then
	for i,one in ipairs(autorelease) do
		delete(one)
	end
	autorelease = {}
end