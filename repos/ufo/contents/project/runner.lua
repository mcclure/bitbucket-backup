-- Additional runners. Orchestrate

global_runner = nil
function next_runner(run)
	if global_run then
		global_run:die()
	end
	global_run = run
	if global_run then
		global_run:insert()
	end
end

local fileversion = 1
gallery = {version=fileversion, best={}}
local gallery_path = "gallery/records.dat"
do
	local data = read_file_table(gallery_path)
	if data and data.version == fileversion then gallery = data end
end
function save_gallery()
	pretty.dump(gallery, gallery_path)
end
function gallery_image_for(key)
	return string.format("gallery/best-%d.png", key)
end

class.FreeRunner(Runner)

function FreeRunner:_init(spec)
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

-- for running two comparison images
class.CoolRunner(Runner)

function CoolRunner:_init(spec)
	self:super(spec)
	
	-- Find correct scale
	if not self.scale then
		self.scale = 1
		while true do
			local scale = self.scale + 1
			local tw, th = self.w*scale*2, self.h*scale
			if tw >= engine.sw or th >= engine.sh then break end
			self.scale = scale
		end
	end

	-- Find correct blit point
	self.sx = engine.sw/4-self.w*self.scale/2
	self.sy = (engine.sh - self.h*self.scale)/2
	self.sx2 = 3*engine.sw/4-self.w*self.scale/2
	self.tx = self.sx
	self.ty = self.sy-engine.fh*2
	self.txright = self.sx2  + self.h*self.scale
	self.titlex = self.sx
	self.titley = self.sy + self.h*self.scale + engine.fh
	self.titlexright = self.txright
end

function CoolRunner:insert(prio)
	Runner.insert(self,prio)
	
	self.coolpic = self.coolpic or MapLoad(self.target or "resource/stage/lesson_2.png")
	if self.tutor then
		table.insert(self.die_with, self.tutor)
		self.tutor.stage = self.stage
		self.tutor:insert()
	end
end

function CoolRunner:die()
	MapKill(self.coolpic)
	
	Runner.die(self)
end

function CoolRunner:remaining()
	local now = engine.sdl.SDL_GetTicks()
	if not self.started then self.started = now end
	local remain = self.started-now + self.limit
--	print(string.format("now %s started %s limit %s remain %s um %s um?? %d\n", now, self.started, self.limit, remain, math.floor(remain/1000), math.floor(remain/1000)))
	if remain < 0 then return nil
	else return string.format("  %d:%02d", math.floor(remain/1000), math.floor((remain%1000)/1000 * 60)) end
end

function CoolRunner:onTick()
	if not self.finished then
		engine.blit_whole_scaled(engine.screen, self.stage.draw, self.scale, self.sx, self.sy)
		engine.blit_whole_scaled(engine.screen, self.coolpic, self.scale, self.sx2, self.sy)
		
		n = project.compute_similarity(self.stage.base, self.coolpic)
		engine.draw_string(string.format("%.2f%% SIMILAR       ", n), self.tx, self.ty)
		if self.best then engine.draw_string_right(string.format("Personal Best: %.2f%%", self.best.similar), self.titlexright, self.titley) end
		if self.limit then
			local remain = self:remaining()
			if remain then
				engine.draw_string_right(remain, self.txright, self.ty)
			else
				self.finished = true
				engine.draw_string_right("COMPLETE!", self.txright, self.ty)
				if not gallery.best[self.scorekey] or gallery.best[self.scorekey].similar < n then
					gallery.best[self.scorekey] = {similar=n, when=os.time()}
					engine.draw_string_right("            NEW BEST", self.titlexright, self.titley)
					save_gallery()
					project.save_image(self.stage.base, gallery_image_for(self.scorekey))
				end
				if self.music then project.sound_release(self.music) self.music = nil end
				project.playsample_set(sampleplayer, ogg_named("time_up"), 1, 0)
				-- YOU WIN!!
			end
		end
		if self.title then engine.draw_string(self.title, self.titlex, self.titley) end
	end
end

require("project/title")