-- Additional Things. On-screne actors

-- Util functions

function game_playsample_set() end -- I DON'T CARE

-- Actual things

local tetros = {
	{{1,1,1,1}},
	{{1,1},{1,1}},
	{{1,1,1},{0,1,0}},
	{{1,1,1},{0,0,1}},
	{{0,0,1},{1,1,1}},
	{{0,1,1},{1,1,0}},
	{{1,1,0},{0,1,1}},
}


local fall = P(0,1)
local left = P(-1,0)
local right = P(1,0)

class.Player(Thing)

function Player:_init(spec)
	pull(self, {
		down = {
		},
		pressed = {
			[engine.sdl.SDL_SCANCODE_LEFT]=function(self) 
				self:try_move(left)
			end,
			[engine.sdl.SDL_SCANCODE_RIGHT]=function(self)
				self:try_move(right)
			end,
			[engine.sdl.SDL_SCANCODE_UP]=function(self)
				self:drop()
			end,
			[engine.sdl.SDL_SCANCODE_DOWN]=function(self)
				self:drop()
			end,
			[engine.sdl.SDL_SCANCODE_SPACE]=function(self)
				self:rotate(1)
			end,
			[engine.sdl.SDL_SCANCODE_A]=function(self)
				self:rotate(-1)
			end,
			[engine.sdl.SDL_SCANCODE_D]=function(self)
				self:rotate(1)
			end,
		},
	d=p0:clone()})
	self:super(spec)
	self.color = engine.color_make(0xFF, 0xFF, 0x7F)
	self.tetro = tetros[math.random(#tetros)]
	self.h = #self.tetro[1]
	self.w = #self.tetro
end

function Player:try_move(way) -- Pointing
	local p = self:at():add(way)
	if self:blocked(p.x,p.y) then return false end
	self:set_at(p)
	return true
end

-- TODO: Comment
function Player:iter(basex,basey)
	basex = basex or self.x
	basey = basey or self.y
	local x, row = nil, nil
	local y, cell = nil, nil
	local function getnext()
		if row then y, cell = next(row, y) end
		while not y do 
			x, row = next(self.tetro, x)
			if not x then return nil end
			y, cell = next(row)
		end
		if cell == 1 then
			return basex+x-math.floor(self.w/2)-1, basey+y-math.floor(self.h/2)-1, cell
		else
			return getnext() -- Tail recurse
		end
	end
	return getnext
end

function Player:blocked(basex,basey)
	for x,y in self:iter(basex,basey) do
		if Thing.blocked(self,x,y) then return true end
	end
	return false
end

function Player:reset()
	self:draw(self.stage.base)
	local clear = {}
	
	local basey = self.y-math.floor(self.h/2)
	for y=basey,basey+self.h-1 do
		-- clear line
		local full = true
		for x=0,self.stage.w-1 do
			if not self.stage:blocked(x,y) then
--				project.set_color(self.stage.base, x, y, engine.color_random())
				full = false
			break end
		end
		if full then table.insert(clear, y) end
	end
	for i,basey in ipairs(clear) do
		for y=basey,1,-1 do
			engine.blit_part(self.stage.base, self.stage.base, 0, y, 0, y-1, self.stage.w, 1)
		end
		engine.draw_rect(0, 0, self.stage.w, 1, engine.black, self.stage.base)
	end
	
	self.owner:reset_player()
end

function Player:drop()
	local p = self:at()
	while true do
		local n = p:add(fall)
		if self:blocked(n.x,n.y) then self:set_at(p) self:reset() return end
		p = n
	end
end

function Player:rotate()
end

function Player:draw(map)
	for x,y in self:iter() do
		project.set_color(map, x, y, self.color)
	end
end

function Player:onTick()
	if self:ticks() % 60 == 0 then
		if not self:try_move(fall) then
			self:reset()
		end
	end
	if not self.blip then
		self:draw(self.stage.draw)
	end
end