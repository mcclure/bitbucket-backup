-- Additional runners. Orchestrate

local fileversion = 1
progress = {version=fileversion, best={}}
local progress_path = "save/score.dat"
do
	local data = read_file_table(progress_path)
	if data and data.version == fileversion then progress = data end
end
function save_progress()
	pretty.dump(progress, progress_path)
end

class.GameRunner(Runner)

function GameRunner:_init(spec)
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

function GameRunner:make_player()
	local player = Runner.make_player(self)
	return player
end

function GameRunner:reset_player()
	doomed(self.player)
	self.player.owner = nil -- Make it easy for the GC.
	self.player = self:make_player()
	table.insert(doom, function() self.player:insert() end)
end


require("project/title")

runners = {
	TitleRunner(),
	GameRunner({w=128, h=128})
}