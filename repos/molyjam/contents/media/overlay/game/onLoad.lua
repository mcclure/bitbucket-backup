-- On Load

-- Some utility functions / classes

function a(v)
	table.insert(autorelease, v)
	return v
end
function r(v)
	table.insert(retain, v)
	return v
end
function i(v)
	return v
end
function vDup(v)
	return Vector3(v.x,v.y,v.z)
end
function vAdd(v,b)
	return a(Vector3(v.x+b.x,v.y+b.y,v.z+b.z))
end
function vSub(v,b)
	return a(Vector3(v.x-b.x,v.y-b.y,v.z-b.z))
end
function vSetPosition(e,v)
	e:setPosition(v.x,v.y,v.z)
end

class "Queue"
function Queue:Queue()
	self.low = 1 self.count = 0
end
function Queue:push(x)
	self[self.low + self.count] = x
	self.count = self.count + 1
end
function Queue:pop()
	if self.count == 0 then
		return nil
	end
	local move = self[self.low]
	self[self.low] = nil
	self.count = self.count - 1
	self.low = self.low + 1
	return move
end
function Queue:peek()
	if self.count == 0 then
		return nil
	end
	return self[self.low]
end

-- Globals
autorelease = {} -- What if already exists?
retain = {}
typed = Queue()
printed = Queue()
tx = 0
ty = 0
entry = ""
at = "start"
mode = 1 -- 0 = entry, 1 = printing
mode_started = ticks
mode_data = {}
cursor_reset = 0
captured = false
blind = false
gameover = false
score = 0

function setmode(x)
	mode = x
	mode_started = ticks
	mode_data = {}
	game[mode]:onStart()
end

-- Setup DOS, shaders

dos = type_automaton()
dos:insert()

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
			if key == KEY_DOWN then c = "DOWN" end
			if key == KEY_UP then c = "UP" end
			if key == KEY_LEFT then c = "Left" end
			if key == KEY_RIGHT then c = "Right" end

			-- Todo: Aggressively ignore anything that isn't ASCII
			if 0<#c and not (key == KEY_ESC or key == KEY_F11 or key == KEY_F12) then
				typed:push(c)
			end
		elseif e:getEventCode() == EVENT_MOUSEDOWN then
			typed:push("Click")
		end
	end
end

do
	local keyer = Keyer()

	Services.Core:getInput():addEventListener(keyer, EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(keyer, EVENT_MOUSEDOWN)
end

function is_prefix_nocase(a,b)
	if #b > #a then return false end
	local x = a:sub(1,#b)
	return x:lower() == b:lower()
end

function miniclear()
	for i=8,22 do
		for j = 2,37 do
			dos:set(j,i," ")
		end
	end
end
function miniclear2()
	for i=8,22 do
		for j = 0,39 do
			dos:set(j,i," ")
		end
	end
end
function getconfig()
	local config = bridge:localf("config.dat")
	if config=="NONE" then
		config = bridge:localf("config.dat.txt")
	end
	if config=="NONE" then
		config = bridge:localf("config.dat.rtf")
	end
	if config=="NONE" then
		config = bridge:localf("config.dat.html")
	end
	if config=="NONE" then
		config = bridge:localf("config.txt")
	end
	if config=="NONE" then
		config = bridge:localf("config.rtf")
	end
	if config=="NONE" then
		config = bridge:localf("config.html")
	end
	if config=="NONE" then
		config = bridge:localf("config.cfg")
	end
	if config=="NONE" then
		config = bridge:localf("config")
	end
	return config
end

game = {
	{ -- 1
		onStart = function(s)
			for i=0,39 do
				dos:set(i,0,"*")
				dos:set(i,23,"*")
			end
			dos:set_centered(0,5,39,"The Shadowland Prophesy")
			dos:set_centered(0,18,39,"RETURN TO START")
			mode_data.input = "a"
		end,
		onKey = function(s, t)
			if t == "\r" then
				setmode(2)
				return true
			else
				mode_data.input = mode_data.input .. t  -- COPYPASTE
				if is_prefix_nocase("return", mode_data.input) then
					if mode_data.input:lower() == "return" then
						if mode_data.input == "Return" then
							setmode(3)
							return false
						else
							dos:set_centered(0,18,39,"No, you have to capitalize \"Return\".")
						end
					end
				else
					mode_data.input = t
				end
			end
			return false
		end,
	},
	{ -- 2
		onStart = function(s)
			dos:set_centered(0,18,39,"No, I mean type \"Return\" to start.")
			mode_data.input = "a"
		end,
		onKey = function(s,t)
			mode_data.input = mode_data.input .. t
			if is_prefix_nocase("return", mode_data.input) then
				if mode_data.input:lower() == "return" then
					if mode_data.input == "Return" then
						setmode(3)
						return false
					else
						dos:set_centered(0,18,39,"No, you have to capitalize \"Return\".")
					end
				end
			else
				mode_data.input = t
			end
		end,
	},
	{ -- 3
		onStart = function(s)
			miniclear2()
			dos:set_centered(0,14,39,"LOADING...")
			mode_data.across = 1
		end,
		onKey = function(s,t)
		end,
		onTick = function(s)
			local x = ""
			for i=1,mode_data.across do
				x = x .. "*"
			end
			for i=mode_data.across,36 do
				x = x .. " "
			end
			for i=1,3 do
				dos:set(2,15+i,x)
			end
			if math.random() < 0.1 then mode_data.across = mode_data.across + 1 end
			if mode_data.across > 36 then setmode(4) end
		end,
	},
	{ -- 4
		onStart = function(s)
			if not s:testConfig() then
				dos:set_centered(0,14,39,"config.dat not found", true)
			end
		end,
		onKey = function(s,t)
		end,
		onTick = function(s)
			if ticks - mode_started == 60*4 then
				dos:set_centered(0,16,39,"You need to create a config.dat file", true)
			end
			if ticks - mode_started == 60*10 then
				dos:set_centered(0,17,39,"Put it in the directory with the game", true)
			end
			if ticks - mode_started == 60*20 then
				dos:set_centered(0,18,39,"config.dat needed to proceed", true)
			end
			if ticks - mode_started == 60*40 then
				dos:set_centered(0,19,39,"Need config.dat. Need it", true)
			end
			s:testConfig()
		end,
		testConfig = function(s)
			local config = getconfig()
			if config~="NONE" then
				setmode(5)
				return true
			end
		end,
	},
	{ -- 5
		onStart = function(s)
			miniclear2()
			if not s:testConfig() then
				dos:set_centered(0,14,39,"okay wait wait", true)
				dos:set_centered(0,15,39,"You call this a config.dat?", true)
			end
		end,
		onKey = function(s,t)
		end,
		onTick = function(s)
			if ticks - mode_started == 60*2 then
				dos:set_centered(0,16,39,"This config.dat is shit", true)
			end
			if ticks - mode_started == 60*4 then
				dos:set_centered(0,18,39,"Tell me a story.                   ", true)
			end
			if ticks - mode_started == 60*4+30 then
				dos:set_centered(0,18,39,"Tell me a story. In the config.dat.", true)
			end
			if ticks - mode_started == 60*10 then
				dos:set_centered(0,19,39,"Oh and it has to be about rabbits.", true)
			end
			s:testConfig()
		end,
		testConfig = function(s)
			local config = getconfig()
			if #config>0 then
				config = config:lower()
				if config:match("bunny") or config:match("bunnies") or config:match("rabbit") then
					setmode(6)
					return true
				end
			end
		end,
	},
	{ -- 6
		onStart = function(s)
			dos:set_centered(0,19,39,"Okay that's great         ")
		end,
		onKey = function(s,t)
		end,
		onTick = function(s)
			if ticks - mode_started == 30 then
				dos:set_centered(0,19,39,"Okay that's great. Hold on", true)
			end
			if ticks - mode_started == 60*2 then
				setmode(7)
			end
		end,
	},
	{ -- 7
		onStart = function(s)
			for i=0,39 do
				dos:set(i,0,"-")
				dos:set(i,23,"-")
			end
			for i=0,23 do
				dos:set(1,i,"|")
				dos:set(38,i,"|")
			end
			for i=2,37 do
				for j=1,7 do
					dos:set(i,j,"%")
				end
			end
			for i=5,34 do
				for j=3,5 do
					dos:set(i,j," ")
				end
			end
			dos:set_centered(5,4,29,"THE SHADOWLAND PROPHESY", true)
			miniclear()
			mode_data.selection = 1
		end,
		onKey = function(s,t)
			if t == "DOWN" then mode_data.selection = mode_data.selection + 1 end
			if t == "UP" then mode_data.selection = mode_data.selection - 1 end
			if mode_data.selection < 1 then mode_data.selection = 1 end
			if mode_data.selection > 3 then mode_data.selection = 3 end
			if t == "\r" then
				if mode_data.selection == 1 then
					dos:set_centered(2, 10, 35, "I really think you should set up")
					dos:set_centered(2, 11, 35, "the options before you try to play.")
				elseif mode_data.selection == 2 then
					dos:set_centered(2, 10, 35, "Obviously you can't continue,")
					dos:set_centered(2, 11, 35, "you've never played before.")
				else
					setmode(8)
				end
			end
			if t == "Click" then
				dos:set_centered(2, 10, 35, "No, no, use the arrow keys.")
				dos:set_centered(2, 11, 35, "          ")
			end
		end,
		onTick = function(s)
			local options = {"Start","Continue","Options"}
			for i=1,3 do
				local o = options[i]
				if i==mode_data.selection then o = "* " .. options[i]:upper() .. " *" end
				dos:set_centered(2, 12+i*2, 35, o)
			end
		end,
	},
	{ -- 8
		onStart = function(s)
			s.tree = {
				options = {
					title = "Options",
					list = {
						{"Video Options", "video"},
						{"Sound Options", "sound"},
						{"Controls", "controls"},
						{"Save", "save"},
					},
				},
				video = {
					title = "Video Options",
					list = {
						{"Pixel Shaders", {type="switch"}},
						{"Geometry instancing", {type="switch"}},
						{"HDR", {type="switch"}},
						{"Back to Options", "options"},
					},
				},
				sound = {
					title = "Sound Options",
					list = {
						{"Volume", {type="number", default=7}},
						{"Sound Effects", {type="number", default=7}},
						{"Voice", {type="number", default=7}},
						{"Back to Options", "options"},
					},
				},
				controls = {
					title = "Controls",
					list = {
						{"Sword", {type="letter", default="z"}},
						{"Fireball", {type="letter", default="x"}},
						{"Aux Lasers", {type="letter", default="c"}},
						{"Back to Options", "options"},
					},
				},
				save = {
					title = "Save Options",
					list = {
						{"Auto-save", {type="switch"}},
						{"File Tag", {type="letter", default="c"}},
						{"Manual save reset", {type="switch", default=false}},
						{"Back", "back"},
					},
				},
				back = {
					title = "Back Options",
					list = {
						{"Spine options", "EXIT"},
						{"Skin options", "EXIT"},
						{"Clothing options", "EXIT"},
						{"Gross Back Hair", "EXIT"},
					},
				},
			}
			miniclear()
			mode_data.at = "options"
			mode_data.selection = 1
			settings = {}
		end,
		onKey = function(s,t)
			local wasarrow = false
			if t == "DOWN" then mode_data.selection = mode_data.selection + 1 wasarrow = true end
			if t == "UP" then mode_data.selection = mode_data.selection - 1 wasarrow = true end
			local limit = #s.tree[mode_data.at].list
			if mode_data.selection < 1 then mode_data.selection = 1 end
			if mode_data.selection > limit then mode_data.selection = limit end
			
			local options = s.tree[mode_data.at]
			local o = options.list[mode_data.selection][1]
			local picked = options.list[mode_data.selection][2]
			if t == "\r" then
				if "table" == type(picked) then
					if picked.type == "switch" then
						settings[o] = not settings[o]
					end
				elseif picked == "EXIT" then
					setmode(9)
				else
					local config = getconfig():lower()
					if picked == "back" and not (config:match("laser") or config:match("bomb") or config:match("lazer")) then
						dos:set_centered(2,8,36,"ERROR: config.dat contains",true,true)
						dos:set_centered(2,9,36,"no references to lasers",true,true)
						mode_data.selection = 1
						mode_data.laserhunt = true
					else
						mode_data.at = picked
						mode_data.selection = 1
						miniclear()
					end
				end
			else
				if not wasarrow and "table" == type(picked) and picked.type ~= "switch" then
					if not (picked.type == "number" and tonumber(t) == nil) then
						settings[o] = t
					end
					if picked.type == "number" and t=="Left" and settings[o] > 0 then settings[o] = settings[o] - 1 end
					if picked.type == "number" and t=="Right" and settings[o] < 9 then settings[o] = settings[o] + 1 end
				else
					if picked.type == "switch" and t=="Left" and not settings[o] then settings[o] = not settings[o] end
					if picked.type == "switch" and t=="Right" and settings[o] then settings[o] = not settings[o] end
				end
			end
		end,
		onTick = function(s)
			local options = s.tree[mode_data.at]
			dos:set_centered(2,10,35,options.title)
			for i,value in ipairs(options.list) do
				local o = options.list[i][1]
				local ext = options.list[i][2]
				if type(ext) == "table" then
					if ext.type == "switch" then
						if settings[o] then o = o .. ": Off" else o = o .. ": On" end
					else
						if not settings[o] then settings[o] = ext.default end
						o = o .. ": " .. settings[o]
					end
				end
				if i==mode_data.selection then o = "* " .. o:upper() .. " *" end
				dos:set_centered(2, 12+i*2, 35, o)
			end
			if mode_data.laserhunt then
				local config = getconfig():lower()
				if config:match("laser") or config:match("bomb") or config:match("lazer") then
					setmode(9)
				end
			end
			
		end,
	},
	
	{ -- 9
		onStart = function(s)
			s.list = {
				{"Video Options", "video"},
				{"Sound Options", "sound"},
				{"Controls", "controls"},
				{"Save", "save"},
				{"Pixel Shaders", {type="switch"}},
				{"Geometry instancing", {type="switch"}},
				{"Volume", {type="number", default=7}},
				{"Sound Effects", {type="number", default=7}},
				{"Voice", {type="number", default=7}},
				{"Sword", {type="letter", default="z"}},
				{"Fireball", {type="letter", default="x"}},
				{"Aux Lasers", {type="letter", default="c"}},
				{"HDR", {type="switch"}},
				{"Auto-save", {type="switch"}},
				{"File Tag", {type="letter", default="c"}},
				{"Manual save reset", {type="switch", default=false}},
				{"Back", "back"},
				{"Spine options", "EXIT"},
				{"Skin options", "EXIT"},
				{"Clothing options", "EXIT"},
				{"Gross Back Hair", "EXIT"},
				{"Geometry Pixels", "EXIT"},
				{"Spline Raster", {type="switch"}},
				{"Geometry Pixels", {type="switch"}},
				{"Vertex Pixels", {type="switch"}},
				{"Shader Vertex", {type="switch"}},
				{"Sandwiches", {type="switch"}},
				{"Wolves", {type="switch"}},
				{"Giant Head Mode", {type="switch"}},
				{"Cheats", {type="switch"}},
				{"Evil", {type="switch"}},
				{"Color Blind mode", {type="switch"}},
				{"Splatter mode", {type="switch"}},
				{"Character name", {type="letter", default="k"}},
				{"Panic button", {type="letter", default="r"}},
				{"Deactivate", {type="letter", default="d"}},
				{"Help", {type="letter", default="x"}},
				{"Sound Effects", {type="number", default=7}},
				{"Size", {type="number", default=7}},
				{"Breadth", {type="number", default=7}},
				{"Girth", {type="number", default=7}},
				{"Appetizers", "sound"},
				{"Foot options", "sound"},
				{"Select Armor", "sound"},
				{"Descend", "sound"},
				{"Broom Closet", "sound"},
			}
			mode_data.selection = 1
			s:setup()
		end,
		onKey = function(s,t)
					local wasarrow = false
			if t == "DOWN" then mode_data.selection = mode_data.selection + 1 wasarrow = true end
			if t == "UP" then mode_data.selection = mode_data.selection - 1 wasarrow = true end
			local limit = #mode_data.list
			if mode_data.selection < 1 then mode_data.selection = 1 end
			if mode_data.selection > limit then mode_data.selection = limit end
			
			local options = mode_data
			local o = options.list[mode_data.selection][1]
			local picked = options.list[mode_data.selection][2]
			if t == "\r" then
				if "table" == type(picked) then
					if picked.type == "switch" then
						settings[o] = not settings[o]
					end
				elseif picked == "REALEXIT" then
					setmode(10)
				else
					mode_data.selection = 1
					s:setup()
				end
			else
				if not wasarrow and "table" == type(picked) and picked.type ~= "switch" then
					if not (picked.type == "number" and tonumber(t) == nil) then
						settings[o] = t
					end
					if picked.type == "number" and t=="Left" and settings[o] > 0 then settings[o] = settings[o] - 1 end
					if picked.type == "number" and t=="Right" and settings[o] < 9 then settings[o] = settings[o] + 1 end
				else
					if picked.type == "switch" and t=="Left" and not settings[o] then settings[o] = not settings[o] end
					if picked.type == "switch" and t=="Right" and settings[o] then settings[o] = not settings[o] end
				end
			end
		end,
		onTick = function(s)
			local options = mode_data
			dos:set_centered(2,10,35,options.title)
			for i,value in ipairs(options.list) do
				local o = options.list[i][1]
				local ext = options.list[i][2]
				if type(ext) == "table" then
					if ext.type == "switch" then
						if settings[o] then o = o .. ": Off" else o = o .. ": On" end
					else
						if not settings[o] then settings[o] = ext.default end
						o = o .. ": " .. settings[o]
					end
				end
				if i==mode_data.selection then o = "* " .. o:upper() .. " *" end
				dos:set_centered(2, 12+i*2, 35, o)
			end
			if mode_data.laserhunt then
				local config = getconfig()
				if config:match("laser") or config:match("bomb") or config:match("lazer") then
					setmode(9)
				end
			end
		end,
		getOption = function(s)
			return s.list[math.random(#s.list)]
		end,
		getExit = function(s)
			while true do
				local a = s:getOption()
				if type(a[2]) ~= "table" then
					return a
				end
				print(type(a[2]))
			end
		end,
		setup = function(s)
			mode_data.title = s:getExit()[1]
			mode_data.list = {s:getOption(),s:getOption(),s:getOption(),s:getOption()}
			local valid = false
			for i=1,4 do
				if type(mode_data.list[i][2]) ~= "table" then
					valid = true
				end
			end
			if not valid then
				print("CORRECTION")
				local a = math.random(4)
				mode_data.list[a] = s:getExit()
			end
			if math.random(5) == 1 then
				local a = math.random(4)
				mode_data.list[a] = {"Return to Title Screen", "REALEXIT"}
			end
			miniclear()
		end,
	},
	
	{ -- 10? END -- Duplicate 7, basically
		onStart = function(s)
			dos:clear()
			for i=0,39 do
				dos:set(i,0,"-")
				dos:set(i,23,"-")
			end
			for i=0,23 do
				dos:set(1,i,"|")
				dos:set(38,i,"|")
			end
			for i=2,37 do
				for j=1,7 do
					dos:set(i,j,"%")
				end
			end
			for i=5,34 do
				for j=3,5 do
					dos:set(i,j," ")
				end
			end
			dos:set_centered(5,4,29,"THE SHADOWLAND PROPHESY", true)
			mode_data.selection = 3
		end,
		onKey = function(s,t)
			if t == "DOWN" then mode_data.selection = mode_data.selection + 1 end
			if t == "UP" then mode_data.selection = mode_data.selection - 1 end
			if mode_data.selection < 1 then mode_data.selection = 1 end
			if mode_data.selection > 3 then mode_data.selection = 3 end
			if t == "\r" then
				if mode_data.selection == 3 then
					setmode(8)
				end
			end
			if mode_data.selection == 1 then
				s:onTick()
				setmode(mode + 1)
			end
		end,
		onTick = function(s)
			local options = {"Start","Continue","Options"}
			for i=1,3 do
				local o = options[i]
				if i==mode_data.selection then o = "* " .. options[i]:upper() .. " *" end
				dos:set_centered(2, 12+i*2, 35, o)
			end
		end,
	},
	
	{ -- END+1
		onStart = function(s)
			mode_data.lines = 0
			mode_data.lines_max = 0
			mode_data.selection = 14
			mode_data.start_at = 12
			dos:set(15, mode_data.selection, "*       *")
			dos:set(17, mode_data.start_at, "Start")
		end,
		onKey = function(s,t)
			local wasArrow = false
			if t == "DOWN" and mode_data.selection < 22 then
				dos:set(15, mode_data.selection, "         ")
				mode_data.lines = mode_data.lines - 1
				mode_data.selection = mode_data.selection + 2
				dos:set(15, mode_data.selection, "*       *")
			end
			if t == "UP" then
				dos:set(15, mode_data.selection, "         ")
				mode_data.lines = mode_data.lines + 1
				if mode_data.selection > 4 then
					mode_data.selection = mode_data.selection - 2
					if mode_data.start_at >= mode_data.selection then mode_data.start_at = mode_data.start_at - 2 end
				else
					dos:rscroll()
					dos:rscroll()
				end
				dos:set(15, mode_data.selection, "*       *")
				dos:set(17, mode_data.start_at, "Start")
			end
			if mode_data.lines_max < mode_data.lines then
				mode_data.lines_max = mode_data.lines
				if mode_data.lines_max == 14 then
					dos:set_centered(0, 14, 24, "Hey, look.")
				end
				if mode_data.lines_max == 17 then
					dos:set_centered(15, 13, 25, "You don't really need to")
					dos:set_centered(15, 14, 25, "play this game.")
				end
				if mode_data.lines_max == 20 then dos:set_centered(0, 11, 40, "You can stay here with me.") end
				if mode_data.lines_max == 25 then dos:set_centered(0, 11, 40, "I could come up with like...") end
				if mode_data.lines_max == 28 then dos:set_centered(0, 11, 40, "I don't know, I have some more ideas.") end
				if mode_data.lines_max == 31 then
					dos:set_centered(0, 11, 40, "Remember the options menu maze?")
					dos:set_centered(0, 12, 40, "That was pretty cool right?")
				end
				if mode_data.lines_max == 34 then
					dos:set_centered(0, 11, 40, "I could tell a whole story.")
					dos:set_centered(0, 12, 40, "With a really big options menu.")
				end
				if mode_data.lines_max == 37 then
					dos:set_centered(0, 11, 40, "There could be puzzles encoded")
					dos:set_centered(0, 12, 40, "in the graphics settings.")
				end
				if mode_data.lines_max == 40 then
					dos:set_centered(0, 11, 40, "I could make a REALLY FANCY")
					dos:set_centered(0, 12, 40, "loading screen. With... shaders?")
				end
				if mode_data.lines_max == 44 then
					dos:set_centered(0, 11, 40, "Would you even need video games?")
					dos:set_centered(0, 12, 40, "If you'd ever had a title screen")
					dos:set_centered(0, 13, 40, "that really cared about you.")
				end
				if mode_data.lines_max == 49 then
					dos:set_centered(0, 11, 40, "Please don't go.")
				end
				if mode_data.lines_max == 54 then
					setmode(mode + 1)
				end
			end
		end,
		onTick = function(s)
		end,
	},
	
	{ -- TEMPLATE
		onStart = function(s)
			dos:set_centered(0, 2, 40, "* START *")
			dos:set_centered(0, 4, 40, "         ")
		end,
		onKey = function(s,t)
			if t == "\r" then
				bridge:crash()
			end
		end,
		onTick = function(s)
			if ticks - mode_started == 1 then
				dos:set_centered(0, 10, 40, "Okay, fine.")
			end
			if ticks - mode_started == 60*2 then
				dos:set_centered(0, 11, 40, "Go ahead. Play the game.")
			end
			if ticks - mode_started == 60*6 then
				dos:set_centered(0, 12, 40, "I bet it sucks anyway.")
			end
		end,
	},
	
	{ -- TEMPLATE
		onStart = function(s)
			dos:set_centered(0,14,39,"Blrgh")
		end,
		onKey = function(s,t)
		end,
		onTick = function(s)
		end,
	},
}

if forcemode then
	setmode(forcemode)
	mode_started = 0
else
	setmode(1)
end