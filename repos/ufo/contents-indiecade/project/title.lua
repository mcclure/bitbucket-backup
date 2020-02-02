class.TitleRunner(Ent) -- Pseudo-runner

local function random_audio(r) r.tape = tapes[math.random(#tapes)] end
local function random_generator(r) r.generator = generators[math.random(#generators)] end
local function return_to_title()
	local run = TitleRunner()
	next_runner(run)
end
local function register_return()
	engine.on_esc = return_to_title
end

artmode = gallery.lastmode or 1

function update_artmode(to)
	artmode = to
	gallery.lastmode = to
	save_gallery()
end

local lesson_titles={
	"Getting started: The blank page",
	"Still life drawing",
	"How to draw a landscape",
	"Introduction to portraits",
	"Mastering facial expressions",
	"Learning from the greats",
}
local lesson_pngs = {
	"resource/stage/lesson_0.png",
	"resource/stage/lesson_1.png",
	"resource/stage/lesson_2.png",
	"resource/stage/lesson_3.png",
	"resource/stage/lesson_4.png",
	"resource/stage/lesson_5.png",
	"resource/stage/scribble.png",
	"resource/stage/pegasister.png",
}

-- ART MODES: 1 Practice 2 Skill 3 Gallery
function TitleRunner:_init(spec)
	pull(self, {
		pressed={
			[engine.sdl.SDL_SCANCODE_P]=function(self) update_artmode(1) end,
			[engine.sdl.SDL_SCANCODE_E]=function(self) update_artmode(2) end,
			[engine.sdl.SDL_SCANCODE_G]=function(self) update_artmode(3) end,
			[engine.sdl.SDL_SCANCODE_S]=function(self)
				local run = FreeRunner()
				random_audio(run) random_generator(run)
				table.insert(doom, function()
					register_return()
					next_runner(run)
				end)
			end,
			[engine.sdl.SDL_SCANCODE_1]=self.load,
			[engine.sdl.SDL_SCANCODE_2]=self.load,
			[engine.sdl.SDL_SCANCODE_3]=self.load,
			[engine.sdl.SDL_SCANCODE_4]=self.load,
			[engine.sdl.SDL_SCANCODE_5]=self.load,
			[engine.sdl.SDL_SCANCODE_6]=self.load,
			[engine.sdl.SDL_SCANCODE_7]=self.load,
			[engine.sdl.SDL_SCANCODE_8]=self.load,
			[engine.sdl.SDL_SCANCODE_9]=self.load,
			[engine.sdl.SDL_SCANCODE_T]=function(self)
				local run = CoolRunner({cursor_blip=true})
				random_audio(run)
				run.generator = ImageGenerator({path=lesson_pngs[math.random(#lesson_pngs)]})
				run.title = "Simply do what the computer does"
				
				local dummy = {}
				random_generator(dummy)
				run.coolpic = dummy.generator:build(128,128)
				
				run.screenshot = run.coolpic
				
				run.tutor = Tutor({x=32, y=32})
				
				-- Create a fake player and submit to the similarity AI all of its functions
				do
					local fakestage = {}
					local fakeplayer = Player({x=32, y=32, stage=fakestage, blip=true}) -- Notice: Not inserted
					local functions = {}
					local want_down = {engine.sdl.SDL_SCANCODE_R, engine.sdl.SDL_SCANCODE_U, engine.sdl.SDL_SCANCODE_P, engine.sdl.SDL_SCANCODE_D, engine.sdl.SDL_SCANCODE_K, engine.sdl.SDL_SCANCODE_B}
					local want_pressed = {engine.sdl.SDL_SCANCODE_H, engine.sdl.SDL_SCANCODE_J} -- No arrow keys
					
					for i,key in ipairs(want_down) do table.insert(functions, fakeplayer.actions.down[key]) end
					for i,key in ipairs(want_pressed) do table.insert(functions, fakeplayer.actions.pressed[key]) end

					run.fakeplayer = fakeplayer -- Protect from garbage collection
					fakeplayer.wrappers = {}

					project.reset_andi_brushes()
					for i,player_function in ipairs(functions) do
						local function wrapper(surface)
							fakestage.base = surface
							player_function(fakeplayer)
						end
						
						table.insert(fakeplayer.wrappers, wrapper)
						project.add_andi_brush(wrapper)
					end
				end
				
				table.insert(doom, function()
					register_return()
					next_runner(run)
				end)
			end,
			[engine.sdl.SDL_SCANCODE_SPACE]=function(self) -- Night games
				local run = NightRunner()
				run.tape = Tape() run.generator = Generator()
				table.insert(doom, function()
					register_return()
					next_runner(run)
				end)
			end,
		}
	})
		
	self:super(spec)
end

function TitleRunner:insert(prio)
	Ent.insert(self, prio)
	engine.clear_screen()
end

function TitleRunner:load(what)
	local idx = what - engine.sdl.SDL_SCANCODE_1 + 1
	
	if artmode == 3 and not gallery.best[idx] then return end
	
	local found = lesson_pngs[idx]
	local title = lesson_titles[idx]
	if found then
		local run = CoolRunner({cursor_blip=(artmode==3), snapshot_eternal=(artmode==2)})
		random_audio(run)
		if artmode == 3 then
			run.generator = ImageGenerator({path=gallery_image_for(idx)})
		else
			random_generator(run)
		end
		run.target = found
		
		if artmode == 2 then run.limit = 10*1000 run.scorekey = idx end
		if artmode ~= 1 then run.best = gallery.best[idx] end
		
		if not title then
			title = ""
			for i=1,16 do title = title .. (math.random()<0.1 and " " or string.char(math.random(127))) end
		end
		run.title = string.format("Lesson %d: %s", idx, title)
		
		table.insert(doom, function()
			register_return()
			next_runner(run)
		end)
	end
end

function TitleRunner:onTick()
	engine.clear_screen()
	
	local modeline = "Pick mode:"

	local titles = {
		"BECOME A GREAT ARTIST IN JUST 10 SECONDS",
		"With Andi and Michael",
		"",
		"Instructions: Press keys on key board.",
		"",
		"         Press space to start",
	}
	--[[
	-- Not in expo version
	for i,v in ipairs(lesson_titles) do
		local best = gallery.best[i]
		if artmode == 1 or (artmode == 2 and not best) then
			table.insert(titles, string.format("(%d) %s", i, v))
		elseif artmode == 2 or (artmode == 3 and best) then
			local base = string.format("(%d) %s", i, v)
			local plus = string.format("%d%%", best.similar)
			for i=1,(#titles[1]-#base-#plus) do base = base .. " " end
			table.insert(titles, base..plus)
		elseif artmode == 3 then
			table.insert(titles,"")
		end
	end
	table.insert(titles,"")
	
	local modes = {"In PRACTICE mode there is no time limit",
	               "Put it to the test in a ten-second EXAM",
   				   "GALLERY of your Exam mode masterpieces!"}
	table.insert(titles, modes[artmode])

	local longest = #modes[2]
--	for i,v in ipairs(modes) do if #v > longest then longest = #v end end
	--]]
	local longest = 0
	for i,v in ipairs(titles) do if #v > longest then longest = #v end end
	local blockwidth = engine.fw*longest
	local blockheight = engine.fh*#titles
	
	local bx,by = (engine.sw-blockwidth)/2, (engine.sh-blockheight)/2
--	print({blockwidth,blockheight,engine.sw,engine.sh,bx,by})
	engine.draw_text( bx, by, titles )
end