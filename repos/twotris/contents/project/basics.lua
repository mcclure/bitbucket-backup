-- Base classes. Runner, Generator, Thing.

pull(km.prio, {stage=km.prio.default-1, magic=1, runner=km.prio.count})

-- Runner management

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

-- Responsible for setting up, tearing down, and displaying to screen stages
-- Requires: generatorFactory
class.Runner(Ent)

function Runner:_init(spec)
	self.original = self.original or spec or {}
	
	pull(self, {w=128, h=128})
	self:super(spec)
end

function Runner:onTick()
	engine.blit_whole_scaled(engine.screen, self.stage.draw, self.scale, self.sx, self.sy)
end

function Runner:make_player()
	return Player({x=1, y=1, stage=self.stage, blip=self.cursor_blip, owner=self})
end

function Runner:insert(prio)
	local e = Ent.insert(self, prio or km.prio.runner)
	engine.clear_screen()
	
	pull(self.original, {tape=self.tape,generator=self.generator})
	
	self.music = self.music or self.tape:build()
	self.stage = self.stage or Stage({w=self.w, h=self.h, base = self.generator:build(self.w,self.h)})
	self.player = self.player or self:make_player()	
	self.die_with = {self.stage, self.player}
	
	self.stage:insert()
	self.player:insert()
	if self.music then project.sound_start(self.music) end
	return e
end

function Runner:die()
	if self.music then project.sound_release(self.music) end
	Ent.die(self)
end

function Runner:clone()
	return getmetatable(self)(self.original)
end

function Runner:rebirth()
	next_runner(self:clone())
end

-- Let's put some wrappers around the surface calls.
function MapMake(w,h) return engine.surfaceSized(w,h) end
function MapCopy(to, from) engine.blit_whole(to, from) end
function MapKill(m) engine.sdl.SDL_FreeSurface(m) end
function MapConvert(i)-- to screen format
	local o = MapMake(i.w,i.h)
	MapCopy(o,i)
	MapKill(i)
	return o
end
function MapLoad(path) return MapConvert(project.load_image(path)) end

function MapBlocked(map,x,y)
	return x < 0 or x >= map.w or y < 0 or y >= map.h or
		project.get_color_x(map,0,x,y)>128 or
		project.get_color_x(map,1,x,y)>128 or
		project.get_color_x(map,2,x,y)>128
end

-- A single "level", a map. Contains a layer of persistent walls (base) and a layer of scratch space for drawing (draw)
-- Self-running. Uses its tick to reset its draw surface to its base.
class.Stage(Ent)

function Stage:_init(spec)
	self:super(spec)
	self.base = self.base or MapMake(self.w,self.h) -- Walls sans Things
	self.draw = self.draw or MapMake(self.w,self.h) -- Walls plus Things
end

function Stage:onTick()
	MapCopy(self.draw, self.base)
end

function Stage:blocked(x,y)
	return MapBlocked(self.base, x, y)
end

function Stage:insert(prio) return Ent.insert(self, prio or km.prio.stage) end

-- An entity on the map
class.Thing(TimedEnt)

function Thing:_init(spec) self:super(spec) end

function Thing:blocked(x,y)
	return self.stage:blocked(x,y)
end

function Thing:at()
	return P(self.x,self.y)
end

function Thing:set_at(p)
	self.x = p.x
	self.y = p.y
end

-- Creates level maps
class.Generator()

function Generator:_init(spec)
	pull(self, spec)
end

function Generator:build(w,h) -- Default implementation-- blank page
	return MapMake(w,h)
end