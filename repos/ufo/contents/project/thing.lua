-- Additional Things. On-screne actors

project.set_brush_type(math.random(9))
project.set_brush_size(math.random(128))

-- Util functions

function skewx(surface, fx, fy, fw, fh, by) -- Negative numbers act funny, idk why
	for y=0,(fh-1) do
		local line = {}
		for x=0,(fw-1) do table.insert(line, project.get_color(surface, fx+x, fy+y)) end
		for x,v in ipairs(line) do
			local off = (x-1+by)%fw
			if off<0 then off = off + fw end
			project.set_color(surface, off, fy+y, v)
		end
	end
end
function skewy(surface, fx, fy, fw, fh, by)
	for x=0,(fw-1) do
		local line = {}
		for y=0,(fh-1) do table.insert(line, project.get_color(surface, fx+x, fy+y)) end
		for y,v in ipairs(line) do
			local off = (y-1+by)%fh
			if off<0 then off = off + fh end
			project.set_color(surface, fx+x, off, v)
		end
	end
end
function skewx_channel(surface, fx, fy, fw, fh, by, channel) -- Negative numbers act funny, idk why
	for y=0,(fh-1) do
		local line = {}
		for x=0,(fw-1) do table.insert(line, project.get_color_x(surface, channel, fx+x, fy+y)) end
		for x,color in ipairs(line) do
			local off = (x-1+by)%fw
--			if off<0 then off = off + fw end -- Only called positive atm
			project.set_color_x(surface, channel, off, fy+y, color)
		end
	end
end


-- Actual things

class.Player(Thing)

function Player:_init(spec)
	pull(self, {
		down = {
			-- NUMBER ROW
			-- `
			-- NUMBERS taken, see pres			
			-- DASH. Equals is TAKEN! see snapshot
			
			-- TOP ROW
			[engine.sdl.SDL_SCANCODE_Q]=function(self) 
				self.y = (self.y - 1 - 1 + self.stage.base.h) % self.stage.base.h + 1
				project.circle_thing(self.stage.base, self.x, self.y)
			end,
			[engine.sdl.SDL_SCANCODE_W]=function(self)
				project.bandit(self.stage.base)
			end,
			[engine.sdl.SDL_SCANCODE_E]=function(self)
				project.find_edges(self.stage.base, 2, 192, engine.color_make(255,0,0,255))
			end,
			[engine.sdl.SDL_SCANCODE_R]=function(self)
				local w,h = self.stage.base.w,self.stage.base.h
				for y=1,h do
					skewx(self.stage.base, 0,y-1,self.stage.base.w, 1,y)
				end
			end,
			[engine.sdl.SDL_SCANCODE_T]=function(self)
				project.do_another_thing(self.stage.base)
			end,
			[engine.sdl.SDL_SCANCODE_Y]=function(self)
				project.do_a_thing(self.stage.base)
			end,
			[engine.sdl.SDL_SCANCODE_U]=function(self) -- Unfortunately mostly a cut and paste of "B"
				local base = self.stage.base
				local w,h = base.w,base.h
				local max = w*h
				local function idx(_i)
					local i = _i-1
					return i%w,math.floor(i/w)
				end
				local function unidx(_i)
					local i = max-_i
					return i%w,math.floor(i/w)
				end
				local function swap(a,b) return a,b end -- don't swap
				local function unswap(a,b) return b,a end -- swap
				local function pass(idxf, swapf)
					local x2,y2 = idxf(1) -- funny naming is so this can double as a "last"
					local c2 = project.get_color(base, x2,y2)
					for i=1,max-1 do
						local x1,y1 = x2,y2
						      x2,y2 = idxf(i+1)
						local c1 = c2
						      c2 = project.get_color(base, x2,y2)
						local lesser, greater = swapf(project.lesser(c1,c2), project.greater(c1,c2))
						project.set_color(base, x1,y1, lesser)
						project.set_color(base, x2,y2, greater)
						c2 = greater
					end
				end
				for i=1,4 do
					pass(idx, swap)
					pass(unidx, unswap)
				end
			end,
			[engine.sdl.SDL_SCANCODE_I]=function(self)
				project.do_a_weird_thing(self.stage.base)
			end,
			[engine.sdl.SDL_SCANCODE_O]=function(self)
				project.find_edges(self.stage.base, 0, 128, engine.color_gray(engine.white))
			end,
			[engine.sdl.SDL_SCANCODE_P]=function(self) -- B for Bubble. Hee hee
				local base = self.stage.base
				local w,h = base.w,base.h
				local max = w*h
				local besides = engine.ticks%2
				local function idxf(_i)
					local i = _i-1+besides
					return i%w,math.floor(i/w)
				end
				for i=1,max-2,2 do
					local x1,y1 = idxf(i)
					local x2,y2 = idxf(i+1)
					local c1 = project.get_color(base, x1,y1)
					local c2 = project.get_color(base, x2,y2)
					project.set_color(base, x1,y1, c2)
					project.set_color(base, x2,y2, c1)
				end
			end, 
			-- [, ], |
			
			-- MIDDLE ROW
			[engine.sdl.SDL_SCANCODE_A]=function(self)
				project.find_edges(self.stage.base, 1, 128, engine.color_make(0,255,0,255))
			end,
			-- S is TAKEN! See down
			[engine.sdl.SDL_SCANCODE_D]=function(self)
				skewx_channel(self.stage.base, 0,0,self.stage.base.w, self.stage.base.h,1, 0) -- Last arg is channel
				skewx_channel(self.stage.base, 0,0,self.stage.base.w, self.stage.base.h,2, 1)
				skewx_channel(self.stage.base, 0,0,self.stage.base.w, self.stage.base.h,3, 2)
			end,
			[engine.sdl.SDL_SCANCODE_F]=function(self)
				project.down_thing(self.stage.base)
			end,
			[engine.sdl.SDL_SCANCODE_G]=function(self)
				project.do_a_locational_thing(self.stage.base, self.x, self.y)
			end,
			-- H is TAKEN! See below. Likewise J.
			[engine.sdl.SDL_SCANCODE_K]=function(self)
				local w,h = self.stage.base.w,self.stage.base.h
				if engine.ticks%4==0 then skewx(self.stage.base, 0,0,self.stage.base.w, self.stage.base.h/2,-1) end
				if engine.ticks%4==1 then skewy(self.stage.base, self.stage.base.w/2,0,self.stage.base.w/2, self.stage.base.h,-1) end
				if engine.ticks%4==2 then skewx(self.stage.base, 0,self.stage.base.h/2,self.stage.base.w, self.stage.base.h/2,1) end
				if engine.ticks%4==3 then skewy(self.stage.base, 0,0,self.stage.base.w/2, self.stage.base.h,1) end
			end,
			[engine.sdl.SDL_SCANCODE_L]=function(self)
				project.magic(self.stage.base)
			end,
			-- semicolon, quote
			
			-- BOTTOM ROW
			[engine.sdl.SDL_SCANCODE_Z]=function(self) 
				self.x = (self.x - 1 - 1 + self.stage.base.h) % self.stage.base.h + 1
				project.circle_thing(self.stage.base, self.x, self.y)
			end,
			-- X is TAKEN! See down
			[engine.sdl.SDL_SCANCODE_C]=function(self)
				self.x = (self.x + 1 - 1 + self.stage.base.h) % self.stage.base.h + 1
				project.circle_thing(self.stage.base, self.x, self.y)
			end,
			-- V is TAKEN, see down
			[engine.sdl.SDL_SCANCODE_B]=function(self) -- B for Bubble. Hee hee
				local base = self.stage.base
				local w,h = base.w,base.h
				local max = w*h
				local function idx(_i)
					local i = _i-1
					return math.floor(i/w),i%w
				end
				local function unidx(_i)
					local i = max-_i
					return math.floor(i/w),i%w
				end
				local function cmp(a,b) return a < b end
				local function uncmp(a,b) return a > b end
				local function pass(idxf, cmpf)
					local x2,y2 = idxf(1) -- funny naming is so this can double as a "last"
					local c2 = project.get_color(base, x2,y2)
					for i=1,max-1 do
						local x1,y1 = x2,y2
						      x2,y2 = idxf(i+1)
						local c1 = c2
						      c2 = project.get_color(base, x2,y2)
						if cmpf(c1,c2) then
							project.set_color(base, x1,y1, c2)
							project.set_color(base, x2,y2, c1)
							c2 = c1
						end
					end
				end
				for i=1,4 do
					pass(idx, cmp)
					pass(unidx, uncmp)
				end
			end,
			[engine.sdl.SDL_SCANCODE_N]=function(self)
				self.y = (self.y + 1 - 1 + self.stage.base.h) % self.stage.base.h + 1
				project.circle_thing(self.stage.base, self.x, self.y)
			end,
			[engine.sdl.SDL_SCANCODE_M]=function(self)
				project.also_do_something_neat_idk(self.stage.base, engine.color_gray(engine.white))
			end,
			[engine.sdl.SDL_SCANCODE_SPACE]=function(self)
				--n = project.increase_similarity(self.stage.base, global_run.coolpic, self.x, self.y)
				--if n==3 then
				--	self.y = (self.y - 1 - 1 + self.stage.base.h) % self.stage.base.h + 1
				--elseif n==2 then
				--	self.x = (self.x - 1 - 1 + self.stage.base.h) % self.stage.base.h + 1	
				--elseif n==0 then
				--	self.x = (self.x + 1 - 1 + self.stage.base.h) % self.stage.base.h + 1
				--else
				--	self.y = (self.y + 1 - 1 + self.stage.base.h) % self.stage.base.h + 1
				--end


				project.playsample_set(sampleplayer, ogg_named("sfx_0"), 1, 0)
				project.circle_thing(self.stage.base, self.x, self.y)
			end,
		},
		pressed = {
			[engine.sdl.SDL_SCANCODE_X]=function(self)
				self.blip = not self.blip
				project.playsample_set(sampleplayer, ogg_named("sfx_5"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_V]=function(self)
				project.textify(self.stage.base)
				project.playsample_set(sampleplayer, ogg_named("sfx_8"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_S]=function(self)
				project.flipline(self.stage.base,self.x, self.y)
				temp = self.y
				self.y = self.x
				self.x = temp
				project.playsample_set(sampleplayer, ogg_named("sfx_4"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_H]=function(self)
				local base = self.stage.base
				local dupe = MapMake(base.w,base.h)
				MapCopy(dupe,base)
				local function inmap(size,x,y)
					x = engine.clamp(1,x*3,size) -- Should be -1 but want smear
					y = engine.clamp(1,y*3,size)
					return x,y
				end
				InPlacePyramid(base, dupe, inmap)
				MapKill(dupe)
				project.playsample_set(sampleplayer, ogg_named("sfx_3"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_J]=function(self)
				local assign = {math.random(3)}
				local color = {0,0,0}
				local base = self.stage.base
				do
					local a,b = (assign[1]-1+1)%3+1, (assign[1]-1+2)%3+1
					if math.random() < 0.5 then table.insert(assign, a) table.insert(assign, b)
					else table.insert(assign, b) table.insert(assign, a) end
				end
				
				for x=0,(base.w-1) do
					for y=0,(base.h-1) do
						for c=1,3 do color[c] = project.get_color_x(base, c-1, x, y) end
						for c=1,3 do project.set_color_x(base, c-1, x, y, color[assign[c]]) end
					end
				end
				project.playsample_set(sampleplayer, ogg_named("sfx_0_anti"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_V]=function(self)
				project.textify(self.stage.base)
				project.playsample_set(sampleplayer, ogg_named("sfx_8"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_MINUS]=function(self)
				local base = self.stage.base
				local subdiv = 4
				local w,h = base.w/subdiv, base.h/subdiv
				local x,y = w*(math.random(subdiv)-1), h*(math.random(subdiv)-1)
				project.playsample_set(sampleplayer, ogg_named("magic1"), 1, 0)
				
				self.die_with = self.die_with or {}
				table.insert(self.die_with,
					Magic({x=x,y=y,w=w,h=h,lifetime=2000,source=base}):insert(km.prio.magic)
				)
				table.insert(self.die_with,
					Anim({x=x,y=y,w=w,h=h,length=8,from=1,to=0,canvas=self.stage.draw,call=function(self,value)
						local color = engine.color_make(0xFF, 0xFF, 0x7F,value*255)
						engine.draw_rect(self.x, self.y, self.w, self.h, color, self.canvas)
					end}):insert()
				)
			end,
			
			[engine.sdl.SDL_SCANCODE_UP]=function(self)
				local base = self.stage.base
				local dupe = MapMake(base.w,base.h)
				MapCopy(dupe,base)
				local function inmap(size,x,y)
					x = engine.clamp(1,size/3+x,size)
					y = engine.clamp(1,size/3+y,size)
					return x,y
				end
				InPlacePyramid(base, dupe, inmap)
				MapKill(dupe)
				project.playsample_set(sampleplayer, ogg_named("sfx_2"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_DOWN]=function(self)
				local base = self.stage.base
				local w,h = base.w, base.h
				local dupe = MapMake(w,h)
				MapCopy(dupe,base)
				local function filter(i,s)
					return math.floor(i/3)+(i%3)*math.floor(s/3)
				end
				for x=0,(w-1) do
					for y=0,(h-1) do
						local c = project.get_color(dupe, x,y)
						project.set_color(base, filter(x,w),filter(y,h), c)
					end
				end
				MapKill(dupe)
				project.playsample_set(sampleplayer, ogg_named("sfx_0"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_LEFT]=function(self)
				local base = self.stage.base
				local w,h = base.w, base.h
				local dupe = MapMake(w,h)
				MapCopy(dupe,base)
				for x=0,(w-1) do
					for y=0,(h-1) do
						local c = project.get_color(dupe, x,y)
						project.set_color(base, (x+math.floor(w/3)-1)%w,y, c)
					end
				end
				MapKill(dupe)
				project.playsample_set(sampleplayer, ogg_named("sfx_2"), 1, 0)
 			end,
			[engine.sdl.SDL_SCANCODE_RIGHT]=function(self)
				local base = self.stage.base
				local w,h = base.w, base.h
				local dupe = MapMake(w,h)
				MapCopy(dupe,base)
				for x=0,(w-1) do
					for y=0,(h-1) do
						local c = project.get_color(dupe, x,y)
						project.set_color(base, x,(y+math.floor(w/3)-1)%h, c)
					end
				end
				MapKill(dupe)
				project.playsample_set(sampleplayer, ogg_named("sfx_2"), 1, 0)
			end,

--function keys
			[engine.sdl.SDL_SCANCODE_RETURN]=function(self)
				--screenshot?
				project.save_image(self.stage.base, string.format("screenshot-%s-%d.png", os.date("%Y.%m.%d-%H.%M.%S"), engine.sdl.SDL_GetTicks()))
				project.playsample_set(sampleplayer, ogg_named("sfx_1"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_1]=function(self)
				project.set_brush_type(1)
				project.playsample_set(sampleplayer, ogg_named("brush_1"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_2]=function(self)
				project.set_brush_type(2)
				project.playsample_set(sampleplayer, ogg_named("brush_2"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_3]=function(self)
				project.set_brush_type(3)
				project.playsample_set(sampleplayer, ogg_named("brush_3"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_4]=function(self)
				project.set_brush_type(4)
				project.playsample_set(sampleplayer, ogg_named("brush_4"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_5]=function(self)
				project.set_brush_type(5)
				project.playsample_set(sampleplayer, ogg_named("brush_5"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_6]=function(self)
				project.set_brush_type(6)
				project.playsample_set(sampleplayer, ogg_named("brush_6"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_7]=function(self)
				project.set_brush_type(7)
				project.playsample_set(sampleplayer, ogg_named("brush_7"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_8]=function(self)
				project.set_brush_type(8)
				project.playsample_set(sampleplayer, ogg_named("brush_8"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_9]=function(self)
				project.set_brush_type(9)
				project.playsample_set(sampleplayer, ogg_named("brush_9"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_0]=function(self)
				project.set_brush_type(0)
				project.playsample_set(sampleplayer, ogg_named("numkey_0"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_F1]=function(self)
				project.set_brush_size(1)
				project.playsample_set(sampleplayer, ogg_named("numkey_1"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_F2]=function(self)
				project.set_brush_size(2)
				project.playsample_set(sampleplayer, ogg_named("numkey_2"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_F3]=function(self)
				project.set_brush_size(4)
				project.playsample_set(sampleplayer, ogg_named("numkey_3"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_F4]=function(self)
				project.set_brush_size(8)
				project.playsample_set(sampleplayer, ogg_named("numkey_4"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_F5]=function(self)
				project.set_brush_size(16)
				project.playsample_set(sampleplayer, ogg_named("numkey_5"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_F6]=function(self)
				project.set_brush_size(24)
				project.playsample_set(sampleplayer, ogg_named("numkey_6"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_F7]=function(self)
				project.set_brush_size(32)
				project.playsample_set(sampleplayer, ogg_named("numkey_7"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_F8]=function(self)
				project.set_brush_size(48)
				project.playsample_set(sampleplayer, ogg_named("numkey_8"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_F9]=function(self)
				project.set_brush_size(56)
				project.playsample_set(sampleplayer, ogg_named("numkey_9"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_F10]=function(self)
				project.set_brush_size(64)
				project.playsample_set(sampleplayer, ogg_named("numkey_10"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_F11]=function(self)
				project.set_brush_size(96)
				project.playsample_set(sampleplayer, ogg_named("numkey_11"), 1, 0)
			end,
			[engine.sdl.SDL_SCANCODE_F12]=function(self)
				project.set_brush_size(128)
				project.playsample_set(sampleplayer, ogg_named("numkey_12"), 1, 0)
			end
		},
	w=2,h=2})
	self:super(spec)
	self.color = engine.color_make(0xFF, 0xFF, 0x7F)
end

function Player:onTick()
	if not self.blip then
		engine.draw_rect(self.x-self.w/2, self.y-self.h/2, self.w, self.h, self.color, self.stage.draw)
	end
end

class.Tutor(Thing)

function Tutor:_init(spec)
	self:super(spec)
end

function Tutor:onTick()
	local n = project.increase_similarity(global_run.coolpic, self.stage.base, self.x, self.y)
	if n==3 then
		self.y = (self.y - 1 - 1 + self.stage.base.h) % self.stage.base.h + 1
	elseif n==2 then
		self.x = (self.x - 1 - 1 + self.stage.base.h) % self.stage.base.h + 1	
	elseif n==0 then
		self.x = (self.x + 1 - 1 + self.stage.base.h) % self.stage.base.h + 1
	else
		self.y = (self.y + 1 - 1 + self.stage.base.h) % self.stage.base.h + 1
	end
end

-- Non-things

class.Snapshot(Ent)

function Snapshot:_init(spec)
	pull(self, {
		down = {
			[engine.sdl.SDL_SCANCODE_DELETE]=self.reset,
			[engine.sdl.SDL_SCANCODE_BACKSPACE]=self.reset,
			[engine.sdl.SDL_SCANCODE_EQUALS]=self.snap
		}
	})
	self:super(spec)
	self.reserve = MapMake(self.onto.w,self.onto.h)
	self:snap()
end

function Snapshot:reset()
	engine.blit_whole(self.onto,self.reserve)
	self.last = engine.sdl.SDL_GetTicks()
	project.playsample_set(sampleplayer, ogg_named("sfx_6"), 1, 0)
end

function Snapshot:snap()
	engine.blit_whole(self.reserve,self.onto)
	self.last = engine.sdl.SDL_GetTicks()
	project.playsample_set(sampleplayer, ogg_named("sfx_7"), 1, 0)
end

function Snapshot:onTick()
	if self.eternal then return end -- No periodic snapshots, only forced ones
	
	local now = engine.sdl.SDL_GetTicks()
	if now-self.last > 10*1000 then
		self:snap()
	end
end

class.Magic(Ent)

function Magic:_init(spec)
	self:super(spec)
end

function Magic:insert(prio)
	self.reserve = MapMake(self.w,self.h)
	self.born = engine.sdl.SDL_GetTicks()
	engine.blit_part(self.reserve, self.source, 0,0, self.x, self.y)
	return Ent.insert(self, prio)
end

function Magic:age()
	return engine.sdl.SDL_GetTicks() - self.born
end

function Magic:onTick()
	if self:age() > self.lifetime then table.insert(doom, function() self:die() end) return end
	engine.blit_whole(self.source, self.reserve, self.x, self.y)
end

function Magic:die()
	if self.reserve then MapKill(self.reserve) self.reserve = nil end
	return Ent.die(self)
end