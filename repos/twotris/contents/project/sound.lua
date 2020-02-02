
project.sound_init()

engine.sdl.SDL_PauseAudio(0)

local cache = {}
function SampleCache(path)
	if not cache[path] then
		cache[path] = project.load_sample(path)
	end
	return cache[path]
end

class.Tape() -- "Generators" but make sounds instead of images

function Tape:_init(spec)
	pull(self,spec)
end

class.SampleTape(Tape)

function SampleTape:_init(spec)	self:super(spec) end

function SampleTape:build()
	local result = project.playsample_make()
	self.play = self.play or SampleCache(self.path)
	project.playsample_set(result, self.play, 1, 1)
	return result
end

class.StutterTape(Tape)

function StutterTape:_init(spec)	self:super(spec) end

function StutterTape:build()
	local result = project.stutter_make()
	self.play = self.play or SampleCache(self.path)
	project.stutter_set(result, self.play, 1, self.every, self.jump)
	return result
end

require("project/s_cutup")

tapes = {
	Tape({build=function(self) end}),
}

-- SOUNDS
square = project.square_make()
project.square_set(square,0,0,0)
project.sound_start(square)

sampleplayer = project.playsample_make()
project.playsample_mute(sampleplayer)
project.sound_start(sampleplayer)

function ogg_name(x) return string.format("resource/audio/%s.ogg", x) end
function ogg_named(x) return SampleCache(ogg_name(x)) end
