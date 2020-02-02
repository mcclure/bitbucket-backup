-- World definition and constants

level_message = {
	"AGENT 7DRL COME IN! YOU SEEK THE MYSTIC ARTIFACT! YOU ARE EARTH'S ONLY~",
	"AGENT 7DRL COME IN! YOU MUST RESCUE MARS AND THEN RETURN TO THE SURFACE!",
	"AGENT 7DRL COME IN! YOU CAN'T MAKE AN OMELETTE WITHOUT KILLING EVERYONE HERE~",
	"AGENT 7DRL COME IN! CAN YOU SURVIVE THE CHALLENGE? RESCUE THE~",
	"AGENT 7DRL COME IN! YOUR OBJECTIVE IS MARS. GOOD LUCK.",
	"AGENT 7DRL COME IN! THROUGH OUR HUBRIS WE HAVE BROUGHT ABOUT MARS! FIND THE~",

	"AGENT 7DRL COME IN! MARS IS HOME TO MANY FRIENDLY PLANTS AND ANIMALS!",
	"AGENT 7DRL COME IN! FOOLISH HUMAN? YOU WILL NEVER SURVIVE MARS!",
	"AGENT 7DRL COME IN! THE EXPERIMENT WAS A FAILURE! REPEAT: ABORT IMMEDIATELY! THE MISSION HAS FAILED!",
	"AGENT 7DRL COME IN! THE EXPERIMENT WAS A SUCCESS! YOU WILL BE DEAD WITHIN MINUTES~",
	"AGENT 7DRL COME IN! THIS WILL BE THE FINAL BATTLE!",
	"AGENT 7DRL COME IN! WE WERE FOOLS TO RISK THE DANGER OF MARS!",

	"AGENT 7DRL COME IN! YOUR MOTHER HAS BEEN CALLING ALL AFTERNOON! WHY DON'T YOU ANSWER YOUR CELL PHONE?",
	"AGENT 7DRL WAKE UP! THE SCENARIO WAS A SIMULATION! YOU HAVE BEEN MANIPULATED~",
	"AGENT 7DRL, PLEASE COME BACK. WE FORGIVE EVERYTHING.",

	"AGENT 7DRL COME IN! THIS IS NO WAY TO MAKE A QUICHE!",

	function() return string.format("PROTIP: YOU MUST DESCEND %d LEVELS AND THEN RETURN TO THE SURFACE.",gm.at_level+1) end,
	"PROTIP: ALTHOUGH THE GAME MAY APPEAR RANDOMLY GENERATED, IN FACT THERE ARE ONLY FOUR~",
	"PROTIP: PRESS X TO JUMP",
	"PROTIP: STOP DOING THAT, THAT'S DISGUSTING",
	"PROTIP: THERE ARE NO PLANTS ON MARS. ANY CREATURES WHICH APPEAR TO BE PLANTS ARE PRETENDING~",
	"PROTIP: TRY TO BEAT THE GAME WITHOUT KILLING ANYTHING. THERE IS NO REASON TO DO SO~",
	shelf_at = 2,
	shelf_height = 6,
}

level_message_amulet = {
	"THE AMULET GLOWS WITH A MYSTERIOUS LIGHT",
	"THE AMULET WAS A SUCCESS. FOOLISH HUMAN? YOU WILL NEVER SURVIVE MARS!",
	"THE AMULET GRANTS YOU THE POWER TO FLY",
	"THE AMULET IS WORTH ALMOST SEVEN DOLLARS",
	"THE AMULET THE AMULET THE AMULET THE AMULET",
	"THE AMULET WAS AN ILLUSION",
	"PROTIP: USE THE AMULET TO ESCAPE",
	"THE AMULET IS YOUR ONLY HOPE",
	shelf_at = 1,
	shelf_height = 1,
}

got_amulet_message = "YOU HAVE FOUND THE AMULET"
won_message = "CONGRATULATIONS. YOU HAVE DONE WHAT NO HUMAN HAS EVER DONE BEFORE: YOU HAVE SAVED MARS."

monster_spec = {
	{"A",
		name="Alien",},
	{"B",
		name="Brony",
		emote={"wants to show you a webcomic"},},
	{i="B",
		name="Wild Brony",
		emote={},},
	{"C",
		name="Corruptor",
		emote={"shimmers"},},
	{"D",
		name="Dust Vortex",},
	{"E",
		name="Emu",},
	{"F",
		name="Martian Fly Trap",},
	{"G",
		name="Gargoyle",},
	{"H",
		name="Floating Head",
		emote={"is weirding you out"},},
	{i="H",
		name="Giant Floating Head",
		emote={"is freaky"},},
	{"I",
		name="Ice Dispenser",
		emote={"bellows mechanically"},},
	{i="I",
		name="Advanced Ice Dispenser",
		emote={"bellows"},},
	{"J",
		name="Jaguar",},
	{"K",
		name="Killer Bee",},
	{"L",
		name="Lost One",},
	{"M",
		name="Mars Monster",
		emote={"is from Mars"}},
	{i="M",
		name="Martian Mars Monster",},
	{i="M",
		name="Mars Monster of Mars",},
	{"N",
		name="Naga",},
	{i="N",
		name="Nagademo",},
	{"O",
		name="Onyx Machine",
		emote={"advances silently"},},
	{"P",
		name="Parasite",},
	{"Q",
		name="Quaristice",},
	{"R",
		name="rock",
		emote={"roars angrily"},},
	{i="R",
		name="angry rock",},
	{"S",
		name="Sludge Vortex",},
	{"T",
		name="Tech Demon",},
	{"U",
		name="Upgrade Treadmill",},
	{"V",
		name="Video Game Reference",
		emote={"MEIN LEIBEN"},},
	{"W",
		name="Windmill",},
	{"X",
		name="X-Virus",},
	{"Y",
		name="Yonic Beast",},
	{"Z",
		name="Z",
		boss = true,
		firetimelow = 5,firetimehigh=60*2, emotetimelow=30, emotetimehigh=60*2,
		hp = 5,
		raw_emote = true,
		emote = {"ZZ ZZZ ZZZ ZZ ZZZZ ZZ",
				 "ZZZZ ZZ ZZ ZZZ ZZ ZZ",
				 "ZZZ ZZZ ZZ ZZZ ZZ Z Z",
				 "Z ZZZZZ ZZ ZZZ ZZ ZZ Z",
				 "ZZZZZ ZZZZZZZ ZZZZZ ZZ",
				 "ZZ Z Z ZZZ Z ZZ ZZZZZ",},},
}

function hitMsg(monster)
	return string.format("You attack the %s!", monster.name)
end

function killMsg(monster)
	return string.format("You defeat the %s!", monster.name)
end

function hurtMsg(monster)
	return string.format("The %s attacks!", monster.name)
end

function emoteMsg(monster)
	local result = monster.emote[math.random(#monster.emote)]
	if monster.raw_emote then
		return result
	else
		return string.format("The %s %s.", monster.name, result)
	end
end

function signalMsg()
	local load_from = gm.won_whole_game and level_message_amulet or level_message
	local messages_upto = gm.at_level < load_from.shelf_at and load_from.shelf_height or #load_from
	local pick = last_pick
	while pick == last_pick do pick = math.random(messages_upto) end
	last_pick = pick
	local result = load_from[pick]
	if type(result) == "function" then result = result() end
	return result
end