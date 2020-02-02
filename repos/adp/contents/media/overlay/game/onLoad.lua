-- On Load

-- Globals
typed = Queue()
printed = Queue()
tx = 0
ty = 0
entry = ""
at = "start"
mode = 0 -- 0 = entry, 1 = printing
cursor_reset = 0
captured = false
blind = false
gameover = false
score = 0
blinded_at = 0
clockticks = 0
clock_message = false

if _DEBUG then -- Preserve room across esc
	if remember_at then at = remember_at end
	function sav() remember_at = at end
end

local dirs = {N="North",E="East",S="South",W="West"}

local magic = {xyzzy=1, plugh=1, plover=1, aybabtu=1, godmode=1, noclip=1, idclip=1, idspispopd=1, abacabb=1, gouranga=1, uuddlrlrbass=1, poweroverwhelming=1, thereisnocowlevel=1, impulse=1, dncornholio=1, dnkroz=1, dnstuff=1, dnitems=1, dnkeys=1, dnweapons=1, dnunlock=1, dninventory=1, dnmonsters=1, dneat=1, dnnuk=1, dnhyper=1, dnscotty=1, dnending=1, dncashman=1, dnview=1, dnskill=1, dnclip=1, dnrate=1, dndebug=1, dncoords=1, dnshowmap=1, dnbeta=1, dncosmo=1, dnallen=1, dntime=1, dntodd=1, rosebud=1, greedisgood=1, iddqd=1, glitteringprizes=1, xaby=1, rbcbcdcu=1, ajppowax=1, barral=1, dyddy=1, } -- Consider "lara" -- 50

-- Setup DOS, shaders

dos = type_automaton()
dos:insert()
dos:displayScene():getDefaultCamera():setPostFilter("FilterMaterial")

do
	shadname = {"x3", "x2", "x1", "x0", "y3", "y2", "y1", "y0"}
	local shadspec = {} for i,name in ipairs(shadname) do shadspec[name] = 0 end
	shader = shaderBindings(dos:displayScene():getDefaultCamera(), shadspec)
end

-- Keyboard handler
class "Keyer" (EventHandler)
function Keyer:Keyer()
	self:EventHandler()
end

function Keyer:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == EVENT_KEYDOWN then
			local c = bridge:charCode(inputEvent)

			-- Windows backspace does not behave like Mac backspace
			if key == KEY_BACKSPACE then c = "\127" end

			-- Todo: Aggressively ignore anything that isn't ASCII
			if 0<#c and not (key == KEY_ESC or key == KEY_TAB or key == KEY_DOWN or key == KEY_UP or key == KEY_LEFT or key == KEY_RIGHT or key == KEY_F11 or key == KEY_F12) then
				typed:push(c)
			end
		end
	end
end

do
	local keyer = Keyer()

	Services.Core:getInput():addEventListener(keyer, EVENT_KEYDOWN)
end

-- DEBUG keyboard handler
class "DebugKeyer" (EventHandler)
function DebugKeyer:DebugKeyer()
	self:EventHandler()
	self.at = 0
end

function DebugKeyer:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == EVENT_KEYDOWN then
			if key == KEY_DOWN or key == KEY_UP then
				local dir = key == KEY_UP and -1 or 1
				self.at = self.at + dir + #shadname
				self.at = self.at % #shadname
				print("Selected " .. shadname[self.at+1])
			elseif key == KEY_LEFT or key == KEY_RIGHT then
				local dir = key == KEY_LEFT and -0.1 or 0.1
				local atshad = shader[shadname[self.at+1]]
				atshad.last = atshad.last + dir
				atshad:set(atshad.last)
				print(shadlast)
			end
		end
	end
end

if _DEBUG then
	local debugkeyer = DebugKeyer()

	Services.Core:getInput():addEventListener(debugkeyer, EVENT_KEYDOWN)
end

-- Game utils

function prompt()
	if not gameover then
		printed:push("\n\n]")
		printed:push({mode=0})
	end
end

function print_broken(str) -- Preprint
	-- Word wrap -- Note: Messes up oddly if a line ends with a newline
	local word = ""
	local linelen = -1 -- This should (?) be 0, but if I set it to 0 things break. What?
	local havespace = false
	local haveany = false
	str = str or "You find yourself in gray mist." -- Error
	
	for c in str:gmatch(".") do
		if c == " " or c == "\r" or c == "\n" then
			wasspace = c == " "
			if linelen + #word >= 39 then
				if haveany and linelen < 39 then printed:push("\n") end
				wasspace = false
			elseif havespace then
				printed:push(" ")
				linelen = linelen + 1
			end
			if not wasspace then
				linelen = 0
				havespace = false
			else
				havespace = true
			end
			printed:push(word)
			linelen = linelen + #word
			word = ""
			haveany = true
			if not wasspace then
				printed:push(c)
			end
		else
			word = word .. c
		end
	end
	if #word then -- Special handling for final word. Could fold in with above...?
		if havespace then
			if linelen + #word >= 39 then
				printed:push("\n")
			else
				printed:push(" ")
			end
		end
		printed:push(word)
	end
end

function blind_check_events()
	local suppress_prompt = false
	-- At last measurement takes 17 steps to win, so let's set a cap of 34 steps.
	local blindtime = clockticks - blinded_at
	print(blindtime)
	if blindtime == 8 then
		print_broken("\n\nYou hear a scratching noise.")
	elseif blindtime == 16 then
		print_broken("\n\nYou hear a loud muffled thud, like the sound of falling earth.")
	elseif blindtime == 24 then
		print_broken("\n\nThe scratching noise is louder now.")
	elseif blindtime == 32 then
		print_broken("\n\nThe scratching is insistent and constant. You can no longer tell if it is even something you are hearing, or just your imagination exaggerating the sound of your own ragged breathing and pounding heart.")
	elseif blindtime == 34 then
		print_broken(string.format("\n\nYou feel a sudden, intense tearing pain as the claw rips into your chest, and then nothing.\n\nYou are dead.\n\nYou received %d of 100 points.\n\nPress esc to quit.\n", score))
		suppress_prompt = true
		gameover = true
	end
	return suppress_prompt
end

function print_room(suppress_prompt) -- Preprint
	local r = rooms[at]
	if not blind then
		local anyexits = false
		
		print_broken(r.text)
		
		printed:push("\n\nExits: ")
		for d,ld in pairs(dirs) do
			if r[d] then
				if anyexits then printed:push(", ") else anyexits = true end
				printed:push(ld)
			end
		end
		if not anyexits then printed:push("None") end
	else
		if r.blindtext then
			print_broken(r.blindtext)
		else
			printed:push("You are in darkness.")
		end
		
		suppress_prompt = blind_check_events()
		if not suppress_prompt and r.blindtext_afterevents then
			print_broken(r.blindtext_afterevents)
		end
	end
	if not suppress_prompt then prompt() end
end

function crlf() -- Postprint
	ty = ty + 1
	tx = 0
	if ty >= 24 then
		dos:scroll()
		ty = 23
	end
end
function splat(s)
	for c in s:gmatch(".") do
		if c == "\r" or c == "\n" then
			crlf()
		else
			dos:set(tx,ty,c) -- Letter
			tx = tx + 1
			if tx >= 40 then crlf() end
		end
	end
end

function scrub(s) -- String mangling
	local r = "" -- Remove all whitespace:
	for x in s:lower():gmatch("%a+") do r = r .. x end 
	return r
end
function string.starts(String,Start)
   return string.sub(String,1,string.len(Start))==Start
end
function is_magic(s)
	return magic[s]
end
function sanitize(s)
	if s then return s else return "(nil)" end
end

function sanity_check() -- Debug -- mechanics start here
	function roomcheck(room, dir, rev)
		local r = rooms[room]
		local exit = r[dir]
		if exit then
			local re = rooms[exit]
			if not re then
				print(string.format("WARN Room %q exit %s says %q, but no such room.", room, dir, exit))
			elseif not re.optout then
				local unexit = re[rev]
				if unexit ~= room then
					print(string.format("WARN Room %q exit %s says %q, but reverse is %q", room, dir, exit, sanitize(unexit)))
				end
			end
		end
	end
	for room, content in pairs(rooms) do
		local text = content.text
		if text and #text>0 and text:sub(-1) ~= "." then
			print(string.format("WARN room %q text doesn't end with period.", room))
		end
		roomcheck(room, "N", "S")
		roomcheck(room, "S", "N")
		roomcheck(room, "W", "E")
		roomcheck(room, "E", "W")
	end
end
function print_map()
	local map = "graph adp {\n"
	for room, content in pairs(rooms) do
		map = map .. room .. "\n"
		for i,d in pairs({"N","E"}) do
			if content[d] then map = map .. room .. " -- " .. content[d] .. "\n" end
		end
	end
	map = map .. "}"
	print(map)
end

-- This emits a .svg file usable by the SVGMaps package from:
-- http://www.ifarchive.org/if-archive/mapping-tools/SVGmaps.zip
-- This package, and the template SVG below, has no included licensing information.
-- I'm choosing not to worry about this because it is not possible to invoke this function
-- in a release build of this game. (TODO: Delete this? This doesn't even work.)
function print_svg_1room(visited, at, x, y)
	local r = rooms[at] if not r or visited[at] then return "" end
	visited[at] = true
	local map = string.format("addRoom(\"%s\",\"%s\",%d,%d)\n", at, at, x, y)
	for d,ld in pairs(dirs) do
		if (d == "N" or d == "E") and r[d] then
			map = map .. string.format("addExit(\"%s\", 0,0,0,1)\n",ld)
		end
	end
	return map
		.. print_svg_1room(visited, r.N, x,y-1)
		.. print_svg_1room(visited, r.S, x,y+1)
		.. print_svg_1room(visited, r.W, x-1,y)
		.. print_svg_1room(visited, r.E, x+1,y)
end
function print_svg()
	local map = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svgll-basic.dtd\"><svg xmlns=\"http://www.w3.org/2000/svg\" xml:space=\"preserve\" width=\"1280\" height=\"1080\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"><script xlink:href=\"definitions.js\" type=\"text/ecmascript\"></script><script xlink:href=\"mapbuilder.js\" type=\"text/ecmascript\"></script><g id=\"map\"></g><script type=\"text/ecmascript\">\n<![CDATA[\n"
	map = map .. "setTab(\"map\")\n"
	map = map .. print_svg_1room({}, "start",0,10);
	map = map .. "setStartRoom(\"start\")\nresizeMap()\n"
	map = map .. "]]>\n</script><g id='ToolTip' visibility=\"hidden\"><rect id=\"ToolTipBox\" rx=\"2\" ry=\"2\" class='ToolTipBox' width=\"200\" height=\"20\" fill=\"yellow\" stroke=\"black\" /><text id=\"ToolTipText\" class='ToolTipText' x='4' y='14' style=\"fill:black;font-family:Verdana,sans serif;font-size:6pt\">hello there!</text></g></svg>"
	print(map)
end

function linkget(t, k) -- linkable get
	if not t then return nil end
	local result = t[k]
	if result and type(result) == "string" and #result > 1 and result:sub(1,1) == "@" then
		result = t[result:sub(2)]
	end
	return result
end

function score_at_least(x) if score < x then score = x end end
function hallucinating() return score > 10 end

function handle(s)
	s = scrub(s)
	if s and #s>0 then
		mode = 1
	
		local match = nil
		local print_after = false
		for d,ld in pairs(dirs) do
			if ld:lower():starts(s) then match = d end
		end
		if match then
			local r = rooms[at]
			clockticks = clockticks + 1
			clock_message = false
			
			if r[match] then
				at = r[match]
				print_after = true
				if rooms[at].onEntry then
					print_after = not rooms[at]:onEntry()
				end
			else
				if blind then
					printed:push("You crash painfully into a wall.")
					
					blind_check_events()
				elseif r.wall and r.wall[match] then
					print_broken(r.wall[match])
				else
					printed:push("A wall blocks the way.")
				end
			end
		elseif string.starts("look",s) then
			print_after = true
		elseif is_magic(s) then
			if hallucinating() then
				print_broken("No god answers your prayer.")
			else
				print_broken("You speak the incantation, but nothing happens.")
			end
		elseif hallucinating() and not blind and s:starts("kill") then
			printed:push("kill kill kill")
		elseif hallucinating() and not blind and s:starts("stop") then
			printed:push("make it stop make it stop make it stop")
		else
			local value = nil
			if not blind then
				local key = s
				if string.starts(s, "look") or string.starts(s, "open") then
					key = s:sub(5)
				end
				value = linkget(rooms[at]["look"], key)
			end
			
			if value then print_broken(value)
			else printed:push("Command not recognized.") end
		end
		
		if print_after then print_room() elseif not gameover then prompt() end
	else
		printed:push("]")
	end
end

-- "Game"

local wbrush = "The brush is too heavy to easily pass that way."
local wriver = "The river blocks your path."
local lriver = "The river is broad, and it's hard to tell from looking how deep it is. Crossing could prove difficult."
local ltarch = {arch="The ceiling supports are brutishly constructed, but suggest a cathedral.", ceiling="@arch",}
function archwith(arg)
	local result = {} pull(result,ltarch) pull(result,arg) return result
end
rooms = {

-- "OUTSIDE"
	start = {
		text = "You are standing outside of the ancestral Curwen estate, an elegant farmhouse built in the 18th century Georgian style. Its majesty is undercut by the long period it has gone unlived-in; paint is peeling and wood is rotted from lack of basic upkeep.",
		S = "unkempt",
		wall = { N="The gate is rusted shut.", W=wbrush, E=wbrush, },
		look = { gate="A chain and antique lock hold the gate shut, but to little real end; the rusted hinges wouldn't move anyway.", lock="@gate", }
	},
	unkempt = {
		text = "You are on an unkempt path.",
		N = "start",
		S = "river",
		wall = { W=wbrush, E=wbrush, },
		look = { brush="Heavy overgrowth surrounds what remains of this narrow path.", path="@brush", growth="@brush", }
	},
	river = {
		text = "A river lazily winds its way here through the overgrowth.",
		N="unkempt", E="entrance",
		wall = { W=wbrush, S=wriver, },
		look = { river=lriver, },
	},
	entrance = {
		text = "Here a sandy riverbank meets a hill that seems to have partially caved in. You find the rotted remnants of what looks like a large wood and metal door, and a dark passageway cuts into the side of the hill.",
		W="river", N="mouth",
		wall = { S=wriver, E=wbrush, },
		look = { river=lriver, door="Rotten wood and metal fragments litter the ground, but on reflection it's not clear what these fragments once were. If this was indeed once a door covering the cave opening to your north, it would have had to have been struck with great force from the inside to scatter the pieces like this.", debris="@door", remains="@door", wood="@door", remnants="@door", hill="The earth slopes upward by the riverbank.", },
		onEntry =
			function (s)
				if captured then
					if blind then score_at_least(100) end
					print_broken(string.format("You step out of the darkness, into daylight.\n\nYou have escaped the catacombs of Joseph Curwen.\n\nYou received %d of 100 points.\n\nPress esc to quit.\n", score))
					gameover = true
					return true
				end
			end,
	},
	mouth = {
		text = "A passage has been cut into the earth here, sloping downward to your north. To your south, daylight is visible.",
		blindtext = "You see daylight before you and feel a rush of outside air.",
		blindtext_afterevents = "\n\nExits: South",
		S="entrance", N="downward",
	},
	downward = {
		text = "The passage grows narrower and slopes down precipitously; you must step carefully, as in places the floor drops a few feet at a time.",
		blindtext = "You are in darkness. You stumble on jagged stairsteps in the floor.",
		S="mouth", N="elbow",
	},
	elbow = {
		text = "Here the earthen passageway terminates at a rock wall, and the passage turns sharply right.",
		look = { wall="A natural rock wall.", },
		S="downward", E="arch",
	},
	tables = { -- Temporary
		text = "A set of tables.",
	},
	
-- Original entrance
	
	arch = {
		text = "You step into a long row of arches and vaulted ceilings; your light does not reach the end of it. Heavy doors of primitive style line the corridor.",
		W="elbow",E="offarch1",N="arch2",
		look=ltarch,
		onEntry = function (s) captured = true end,
	},
	offarch1 = {
		text = "A room littered with debris. Instruments of unrecognizable function lay scattered in disassembled pieces. There is what looks like a fireplace in the corner.",
		W="arch",
	},
	
	arch2 = {
		text = "A long row of arches and vaulted ceilings stretches to the north and south. Heavy doors line the corridor.",
		S="arch",W="to_house",E="study",
		look=archwith({debris="The ceiling seems to have caved in partially.", pile="@debris",}),
		wall = {N="A pile of debris blocks the way."},
	},
	to_house = {
		text = "A narrow earthen tunnel. You must stoop slightly to pass through.",
		E="arch2",W="house",
	},
	house = {
		text = "You find yourself at the bottom of a shaft-like room; at one corner the remains of what seem to have once been stone steps lead upward. Above is a solid wooden ceiling.",
		E="to_house",
	},
	
	study = {
		text = "A sort of study. Compared to the dusty ruin of the other rooms, the room contains some recognizably modern furnishings and shows signs of more recent occupation. Papers and books lay across the floor in disarray.",
		W="arch2", E="arch3",
	},
	
	arch3 = {
		text = "A long row of arches and vaulted ceilings stretches to the north and south. Heavy doors line the corridor.",
		W="study",E="offarch2",N="arch4",S="arch5",
	},	
	offarch2 = {
		text = "This room contains only a pile of empty storage crates, mostly cracked and rotted.",
		W="arch3",
	},
	
	arch4 = {
		text = "A long row of arches and vaulted ceilings stretches to the south. To the north is an alcove where steps lead upward.",
		N="outbuilding",S="arch3",E="offarch3",
	},
	offarch3 = {
		text = "Lead coffins lay stacked in several rows here. They look heavy, almost as if they were armored.",
		W="arch4",
	},
	outbuilding = {
		text = "The staircase spirals up into a narrow cylinder; you climb up for quite some ways before reaching a spot where the stairs have caved in.",
		S="arch4",
	},
	
	arch5 = {
		text = "A long row of arches and vaulted ceilings stretches to the north and south. Heavy doors line the corridor.",
		N="arch3", W="chamber1", E="lab1",
	},
	chamber1 = {
		text = "Stored here is what looks like a large amount of antique clothing, packed together in large bales. It has clearly not been disturbed in a long time.",
		E="arch5",
	},

-- Lab
	lab1 = {
		text = "You walk into some sort of complexly furnished laboratory, of recent-- or at least more recent than the other rooms-- occupation. Someone was busy here.",
		E="lab2",N="laboff1",
		W="arch5",
	},
	lab2 = {
		text = "Tables covered in papers and turn-of-the-century scientific apparatus. Books on what appear to be occult subjects lay open, some in languages you cannot identify.",
		E="lab3",W="lab1",N="laboff2",
	},
	lab3 = {
		text = "A large operating table with heavy leather arm and foot harnesses. The metal is interlaced with strange stains and corruption.",
		W="lab2",N="laboff3",
		E="arch6"
	},
	laboff1 = {
		text = "Boxes of clothing and stored goods, and a collection of empty coffins, of varying styles but most badly damaged.",
		S="lab1",
	},
	laboff2 = {
		text = "A pile of trash, mostly consisting of what actually seems to be antique chemist's equipment. In one corner of the room are a collection of boxes, tightly nailed shut.",
		S="lab2",
	},
	laboff3 = {
		text = "A long chamber, lined with shelves on each side. On the shelves are hundreds of meticulously organized jars, each tagged with a number. Above the shelves on the west side are a sign reading \"CUSTODES\" and on the east side, \"MATERIA\".",
		S="lab3",N="laboff3_2",
	},
	laboff3_2 = {
		text = "The shelves terminate at a small door, around which an acrid chemical smell lingers. Chiseled above the door is a sinister-looking geometric symbol.",
		S="laboff3",N="laboff3_3",
	},
	laboff3_3 = {
		text = "The smell of chemicals in this room is overpowering, enough it almost distracts from the offputting contents of the room itself. The room is dominated by a pentagram engraved into the floor, with other circles and geometric figures madly overlaid. Whips and medieval instruments of torture are lined in racks on the walls. At the far end is a small, orderly table and chair.",
		S="laboff3_2"
		-- TODO
	},

	arch6 = {
		text = "A long row of arches and vaulted ceilings stretches to the east. Heavy doors line the corridor.",
		W="lab3",S="chamber4",E="arch7",
	},
	chamber4 = {
		text = "Some sort of chemical workshop. On a series of wood tables rest lead bowls of various sizes and with cryptic signs drawn on the sides. A cacophony of intense and unpleasant odors lingers around the bowls. In a corner is a large copper vat.",
		N="arch6",
	},
	
	arch7 = {
		text = "A long row of arches and vaulted ceilings stretches to the east. Heavy doors line the corridor.",
		W="arch6",S="chamber3",N="stonehenge_s",E="arch8",
	},
	chamber3 = {
		text = "A long room containing a row of copper vats, their industrial appearance jarring with the mystic symbols etched into the side.", -- Engraved?
		N="arch7",
	},

	arch8 = {
		text = "A long row of arches and vaulted ceilings stretches to the east and west. Heavy doors line the corridor.",
		W="arch7",S="chamber2",E="barracks",
	},
	chamber2 = {
		text = "A collection of clothes and survival goods, haphazardly stacked as if just delivered. It looks like more supplies than one person could possibly need.",
		N="arch8",
	},
	barracks = {
		text = "A long room which resembles nothing more than a military-style barracks.",
		W="arch8",
	},

	arch3off1 = {
		text = "A stone room, empty except for dirt."
	},
		
-- "ENDING"
	stonehenge_s = {
		text = "The narrow passageways suddenly give way to a great open space. Your light barely illuminates a large stone structure to your north.",
		N="stonehenge", W="stonehenge_sw", E="stonehenge_se",
		S="arch7", -- TBD
	},
	stonehenge_sw = {
		text = "The room curves in a circle, around large stone pillars supporting the arched roof.",
		N="stonehenge_w", E="stonehenge_s",
	},
	stonehenge_se = {
		text = "The room curves in a circle, around large stone pillars supporting the arched roof.",
		N="stonehenge_e", W="stonehenge_s",
	},
	stonehenge_w = {
		text = "To your west is a large black doorway, which will not open. To your east is a stone structure.",
		E="stonehenge",S="stonehenge_sw",N="stonehenge_nw",
	},
	stonehenge_e = {
		text = "The wall is lined with shallow cells; through the iron grating you can see manacles affixed to the walls. To your west is a stone structure.",
		W="stonehenge",N="stonehenge_ne",S="stonehenge_se",
	},
	stonehenge = {
		text = "A circle of standing stones, arranged as if to mimic Stonehenge. At the center on a circular dais is a sort of altar, covered in unpleasant carvings and even more unpleasant stains. To your south is the door you entered through. To your west is a heavy-looking black door. To your east is a series of alcoves. To your north a large gaping portal draws your attention.",
		blindtext = "You stumble over a step in the floor.",
		E="stonehenge_e",N="stonehenge_n",W="stonehenge_w",S="stonehenge_s",
	},
	stonehenge_nw = {
		text = "The room curves around to a large opening to your east.",
		S="stonehenge_w",E="stonehenge_n",
	},
	stonehenge_ne = { -- + stonehenge_ne
		text = "The room curves around to a large opening to your east.",
		S="stonehenge_e",W="stonehenge_n",
	},
	stonehenge_n = {
		text = "To your north is a large, open doorway that is more like a hole. The lines of the room seem to all converge toward this opening. To your south is a stone structure.",
		N="star", W="stonehenge_nw", E="stonehenge_ne", S="stonehenge",
	},
	star = {
		text = "You are in a star-shaped room. On all sides are dark chambers. The door through which you entered lies to the south.",
		S="stonehenge_n",E="horror",N="horror",W="horror",
		onEntry = function (s) score_at_least(50) end,
	},
	horror = {
		text = "",
		optout=true,
		onEntry =
			function (s)
				if blind then
					print_broken("A rush of air alerts you, and just in time you catch yourself from falling into the pit. You stumble backward.\n\n")
				else
					print_broken("You inch forward to the edge of the pit. Your glimpse at what you see below lasts only a few moments, but feels like an eternity. You scream, drop your light, flailing stumble backward into the room behind you.\n\n")
					blind = true
					blinded_at = clockticks
					score_at_least(75)
				end
				at="star"
				print_room(true)
				return true
			end,
	},
}

-- Debug: check sanity
if _DEBUG then sanity_check() end

-- Run
print_room()