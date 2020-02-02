-- Additional Things. On-screne actors

-- Util functions

function game_playsample_set() end -- I DON'T CARE

-- Actual things

class.Player(Thing)

function Player:_init(spec)
	pull(self, {
		down = {
			[engine.sdl.SDL_SCANCODE_D]=function(self) self:travel_raw(1) end,
			[engine.sdl.SDL_SCANCODE_W]=function(self) self:travel_raw(2) end,
			[engine.sdl.SDL_SCANCODE_A]=function(self) self:travel_raw(3) end,
			[engine.sdl.SDL_SCANCODE_S]=function(self) self:travel_raw(4) end,
			
			[engine.sdl.SDL_SCANCODE_LEFT]=function(self) 
				if self:still() then self:set_next(-1) end
			end,
			[engine.sdl.SDL_SCANCODE_RIGHT]=function(self)
				if self:still() then self:set_next(1) end				
			end,
		},
		pressed = {
			[engine.sdl.SDL_SCANCODE_UP]=function(self)
				if self:still() then self.d = self:away() end
			end,
			[engine.sdl.SDL_SCANCODE_DOWN]=function(self)
			end,
		},
	w=1,h=1,d=p0:clone()})
	self:super(spec)
	self.color = engine.color_make(0xFF, 0xFF, 0x7F)
end

function Player:travel_raw(dir) -- For debugging
	local to = pos(self):travel_dir(dir):wrap(size(self.stage))
--	if self:blocked(to.x,to.y) then return end
	set_pos(self, to)
end

function Player:open() -- Which of the four compass directions is currently accessible? Return a hash
	local p = pos(self)
	local s = size(self.stage)
	local found = {}
	for i=1,4 do
		local p2 = p:travel_dir(i)
		if p2:within(s) and not self:blocked(p2.x,p2.y) then found[i]=true end
	end
	return found
end

function Player:away() -- Find a single direction which is "unblocked", away from the cursor
	local p = p0
	local open = self:open()
	for i=1,4 do
		if open[i] and not open[mod1(i+2,4)] then
			p = p:travel_dir(i)
		end
	end
	return p
end

function Player:set_next(way) -- Pointing 
	local p = pos(self)
	local open = self:open()
	local ndir = reverse_semidir(self.normal)
--	print("-------------------\n\n---------------")
--	print({at=p, normal=self.normal, ndir=ndir})
	for i=1,8 do
		local at = mod1(ndir + i*way, 8)
--		print({i=i, trying=at})
		local check = p:travel_semidir(at)
--		print({check=check,blocked=self:blocked(check.x,check.y)})
		if self:blocked(check.x,check.y) then
			self.normal = p0:travel_semidir(at)
		else
			if at%2==1 then
				local newnormal = p:add(self.normal):subtract( check )
--				print({oldroot=p:add(self.normal), newnormal=newnormal})
				set_pos(self, check)
				self.normal = newnormal:norm()
				return
			end
		end
	end
end

function Player:still()
	return self.d.x==0 and self.d.y==0
end

function Player:randomize()
	self.x = math.random(self.stage.w)
	self.y = math.random(self.stage.h)
	self.d = P(math.random(-1,1), math.random(-1,1))
	if self:still() then
		self:randomize() -- TAIL
	else
		local nx,ny = self.x,self.y
		while self:blocked(nx,ny) or self:blocked(nx+1,ny) or self:blocked(nx-1,ny) or self:blocked(nx,ny+1) or self:blocked(nx,ny-1) do
			nx = nx + 1
			if nx > self.stage.w then
				nx = 1
				ny = ny + 1
				if ny > self.stage.h then
					ny = 1
				end
			end
			if self.x==nx and self.y==ny then print("FAIL") break end -- TODO do something on fail
		end
		self.x,self.y = nx,ny
	end
end

function Player:do_velocity()
	local nx,ny = self.x+self.d.x,self.y+self.d.y
	if nx < 1 or nx>self.stage.w then -- Bounce X
		self.d.x = -self.d.x
		self:do_velocity() -- TAIL
	elseif ny < 1 or ny>self.stage.h then -- Bounce Y
		self.d.y = -self.d.y
		self:do_velocity() -- TAIL
	elseif self:blocked(nx,ny) then -- Come to rest
		self.normal = self.d
		self.d = p0:clone()
	else -- Fly
		self.x,self.y = nx,ny
	end
end

function Player:onTick()
	if not self:still() then
		self:do_velocity()
	end
	if not self.blip then
		project.set_color(self.stage.draw, self.x, self.y, self.color)
	end
end