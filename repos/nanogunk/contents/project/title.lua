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

-- ART MODES: 1 Practice 2 Skill 3 Gallery
function TitleRunner:_init(spec)
	pull(self, {
		pressed={
			[engine.sdl.SDL_SCANCODE_SPACE]=function(self)
				local run = FreeRunner()
				random_audio(run) random_generator(run)
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

function TitleRunner:onTick()
	engine.clear_screen()
	
	local titles = {
		"   Nanogunk   ",
		"", "", "", "", "",
		"Press up arrow",
	}
		
	local longest = 0
	for i,v in ipairs(titles) do if #v > longest then longest = #v end end
	local blockwidth = engine.fw*longest
	local blockheight = engine.fh*#titles
	
	local bx,by = (engine.sw-blockwidth)/2, (engine.sh-blockheight)/2
--	print({blockwidth,blockheight,engine.sw,engine.sh,bx,by})
	engine.draw_text( bx, by, titles )
end