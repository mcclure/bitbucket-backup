-- On Load

pull(km, {tempo = 8, boardw = 60, boardh=20, gfxoff = 0, tilesize = 8, tilecount = 5, growprob = (not hasplayed) and 2 or math.random(2,3), playerprob=5, growbuf=2,})
pull(km, {gfxh = km.boardw*km.tilesize + km.gfxoff*2, gfxw = km.boardh*km.tilesize})
pull(gm, {lastTick=0, clickers={}})

hasplayed = true

function pxy(x, y)
	x = x - 1
	y = y - 1
	return x*km.tilesize+km.gfxoff, y*km.tilesize
end

function make_image(sprite, context)
	local name = string.format("media/%s/%s.png", context or "sprite", sprite)
	local g = gfxcontainer()
	local success = g:load_image(name)
	if success then return r(g) end
	delete(g) return nil
end

tile = {}
for i=1,km.tilecount do
	tile[i] = make_image(string.format("tile%d", i), "tile")
end

au = {}
do
       local data = a(NumberArray())
       local limit = (1024*4)-1
       local v
       for i=0,limit do
               if i % 1 == 0 then
                       v = math.random()*2-1
               end
               data:push_back(v*((limit-i)/limit))
       end
       au[1] = r(bridge:soundFromValues(data))
	   
	   local data2 = a(NumberArray())
	   for i=0,limit do
               data2:push_back(data:get(limit-i-1))
       end
	   au[2] = r(bridge:soundFromValues(data2))
end

killDos(false, km.gfxw, km.gfxh)
dos = type_automaton()
gfx = dos:toGfx()
dos:insert()

class "Clicker" (Ent)

function Clicker:Clicker(spec)
	Ent.Ent(self, spec)
end

function Clicker:insert()
	table.insert(gm.clickers, self)
	return Ent.insert(self)
end

function randP()
	return P(math.random(km.boardw), math.random(km.boardh))
end

function rotate(i, d)
	i = i - 1
	i = i % d
	i = i + 1
	return i
end

function valid(p)
	return p.x >= 1 and p.y >= 1 and p.x <= km.boardw and p.y <= km.boardh
end

class "Grower" (Clicker)

function Grower:Grower(spec)
	pull(self, { pending = {}, max = 0, growprob = km.growprob, color=0xFFFFFFFF})
	Clicker.Clicker(self, spec)
	self.grid = self.grid or SparseGrid()
end

function Grower:newP(p)
	local kind = math.random(km.tilecount)
	self.grid:set(p.x,p.y, kind)
	table.insert(self.pending, p)
	
	if p.x > self.max then self.max = p.x end
	
	local px, py = pxy(p.x,p.y)
	gfx:pxcopy_recolor(tile[kind], self.color, px, py)
end

function Grower:onClick()
	local oldpending = self.pending
	self.pending = {}
	for _, p in ipairs(oldpending) do
		local freecount = 0
		local free = {}
		for idx = 1,4 do
			local ip = p:travelidx(idx)
			if valid(ip) then
				local is = self.grid:get(ip.x, ip.y)
				if is then
					is = false
				else
					is = ip
					freecount = freecount + 1
				end
				table.insert(free, is)
			end
		end
		while (freecount > 0 and math.random(self.growprob) == 1) do
			local picked = math.random(4)
			while (not free[picked]) do picked = rotate(picked + 1, 4) end
			self:newP(free[picked])
			free[picked] = false
			freecount = freecount - 1
		end
		if (freecount > 0) then
			table.insert(self.pending, p)
		end
	end
end

function Grower:scroll(by)
	local grid = SparseGrid()
	for p,v in self.grid:iter() do
		if p.x > by then
			grid:set(p.x-by, p.y, v)
		end
	end
	for _, v in ipairs(self.pending) do v.x = v.x - by end
	self.grid = grid
	self.max = self.max - by
end

class "MapGrower" (Grower)

function MapGrower:MapGrower(spec)
	Grower.Grower(self, spec)
	for y=1,km.boardh do
		self:newP(P(1,y))
	end
end

function MapGrower:onClick()
	Grower.onClick(self)
end

class "PlayerGrower" (Grower)

function PlayerGrower:PlayerGrower(spec)
	spec.growprob = spec.growprob or km.playerprob
	Grower.Grower(self, spec)
end

function PlayerGrower:onClick()
	Grower.onClick(self)
end

g = MapGrower()
g:insert()

local sx, sy = math.floor(km.boardw/4), math.floor(km.boardh/4)

p1 = PlayerGrower({color=0x0077FF})
p1:newP(P(sx, sy))
p1:insert()

p2 = PlayerGrower({color=0xFF7700})
p2:newP(P(sx, sy*3))
p2:insert()