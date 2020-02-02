-- Additional runners. Orchestrate

local fileversion = 1
progress = {version=fileversion, best={}}
local progress_path = "save/progress.dat"
do
	local data = read_file_table(progress_path)
	if data and data.version == fileversion then progress = data end
end
function save_progress()
	pretty.dump(progress, progress_path)
end

class.FreeRunner(Runner)

function FreeRunner:_init(spec)
	self.original = spec
	pull(self, {
		pressed={
			[engine.sdl.SDL_SCANCODE_GRAVE]=function(self) 
				self:rebirth()
			end,
		},
	})
	self:super(spec)
	
	-- Find correct scale
	if not self.scale then
		self.scale = 1
		while true do
			local scale = self.scale + 1
			local tw, th = self.w*scale, self.h*scale
			if tw >= engine.sw or th >= engine.sh then break end
			self.scale = scale
		end
	end
	
	-- Find correct blit point
	self.sx = (engine.sw - self.w*self.scale)/2
	self.sy = (engine.sh - self.h*self.scale)/2
end

function FreeRunner:make_player()
	local player = Runner.make_player(self)
	player:randomize()
	return player
end

require("project/title")