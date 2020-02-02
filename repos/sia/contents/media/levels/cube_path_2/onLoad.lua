addScreen("cube_path_2", km.roomObj, true)

au.rainbow:setVolume(0.02)

local bots = {}
for i=1,3 do
	table.insert(bots, string.format("dbot%d",i))
end

if fm.robotdead then
	for i,v in ipairs(bots) do
		bridge:setVisible(id(v),false)
	end
end

plantScreen(screens["cube_path_2"])