-- Program

require( "project/startup" )
require( "project/ent" )
require( "project/engine" )
require( "project/sound" )
require( "project/basics" )
require( "project/generator" )
require( "project/thing" )
require( "project/runner" )

-- Load

slideshow_index = "slideshow/index.txt"
slideshow = {}
do
	local slideshow_index_file = io.open(slideshow_index, "r")
	if slideshow_index_file then
		while true do
			local slideshow_file = slideshow_index_file:read() -- 1 line
			if slideshow_file == nil then break end
			table.insert(slideshow, slideshow_file)
		end
		slideshow_index_file:close()
	end
end
print(slideshow)

-- Run

pull(gm, {r=Router():insert()})
pull(km, {count=8, distfactor=math.sqrt(engine.sh*engine.sh + engine.sw*engine.sw)/4, })

function notzero(n) local a = tonumber(n) if a ~= 0 then return a end return nil end

local pickgen = notzero(arg[1]) or math.random(#generators)
local picktape = notzero(arg[2]) or math.random(#tapes)
local pickrunner = tonumber(arg[3]) or 1

local runners = {
	TitleRunner(),
	FreeRunner(),
	CoolRunner(),
	FreeRunner({w=512,h=512}),
}

local run = runners[pickrunner]
run.generator = generators[pickgen]
run.tape=tapes[picktape]
next_runner(run)

function engine.render()
	entity_tick()
	engine.sdl.SDL_Flip(engine.screen)
end

engine.loop()

engine.sdl.SDL_PauseAudio(1) -- Stop audio thread