-- Game over screen setup
titleDone = false

dos = type_automaton()
dos:insert()

local killer_name = gm.killed_by.name:gsub(" ","~")
local killer_article = killer_name:match("^[aeiouAEIOU]") and "n" or ""

dos_write = {
	{ 60*1, 6,  "You~are~dead", },
	{ 60*2, 10,  string.format("Killed~on~level~%d", gm.at_level), },
	{ 60*3, 11, string.format("by~a%s~%s", killer_article, killer_name) },
	{ 60*4, 18, "Press~a~button", },
}

if gm.won_whole_game then
	table.insert(dos_write, 
		{ 60*4, 12, "after~obtaining~the~Amulet" }
	)
	dos_write[4][1] = 60*5
end

-- Keyboard handler
do
	input = TitleInput()

	Services.Core:getInput():addEventListener(input, EVENT_KEYDOWN)
end