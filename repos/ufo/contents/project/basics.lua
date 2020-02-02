-- Base classes. Runner, Generator, Thing.

pull(km.prio, {stage=km.prio.default-1, magic=1, runner=km.prio.count})

-- Responsible for setting up, tearing down, and displaying to screen stages
-- Requires: generatorFactory
class.Runner(Ent)

function Runner:_init(spec)
	pull(self, {w=128, h=128})
	self:super(spec)
end

function Runner:onTick()
	engine.blit_whole_scaled(engine.screen, self.stage.draw, self.scale, self.sx, self.sy)
end

function Runner:insert(prio)
	local e = Ent.insert(self, prio or km.prio.runner)
	engine.clear_screen()
	
	self.music = self.music or self.tape:build()
	self.stage = self.stage or Stage({w=self.w, h=self.h, base = self.generator:build(self.w,self.h)})
	self.player = self.player or Player({x=32, y=32, stage=self.stage, blip=self.cursor_blip})
	self.snapshot = self.snapshot or Snapshot({onto=self.stage.base, eternal=self.snapshot_eternal})
	
	self.die_with = {self.stage, self.player, self.snapshot}
	
	self.stage:insert()
	self.player:insert()
	self.snapshot:insert()
	project.sound_start(self.music)
	return e
end

function Runner:die()
	if self.music then project.sound_release(self.music) end
	Ent.die(self)
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

function Stage:passable(x,y)
	return project.get_component(project.get_color(self.draw, x,y),0)<128
end

function Stage:insert(prio) return Ent.insert(self, prio or km.prio.stage) end

-- An entity on the map
class.Thing(Ent)

function Thing:_init(spec) self:super(spec) end

-- Creates level maps
class.Generator()

function Generator:_init(spec)
	pull(self, spec)
end

function Generator:build(w,h) -- Default implementation-- blank page
	return MapMake(w,h)
end