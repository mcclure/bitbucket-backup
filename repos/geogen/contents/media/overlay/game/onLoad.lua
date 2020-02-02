-- On Load

km = {circhigh = 3, circlow = sqrt(2)/2, steps = 65,
}

-- Set up "spotlight" effect
--screen():setScreenShader("FilterMaterial")
--shader = shaderBindings(screen(), 
--	{center=Vector3(0.1,0.1,0.0), radius=0.1, intensity=100.0, aspect=(surface_width/surface_height)})l

class "GeoGenerator" (Generator)

function GeoGenerator:GeoGenerator(namer)
	self:_init(namer)
	self.s = self:basicScreen()
	
	self.shapes = {}
end

function GeoGenerator:rcolor(shape)
	shape:setColor(math.random(),math.random(),math.random(),1)
end

function GeoGenerator:purge()
	for i, shape in ipairs(self.shapes) do
		self.s:removeChild(shape)
	end
	self.shapes = {}
end

function GeoGenerator:build()
	self:purge()
	for i=1,10 do
		local p = Polygon()
		for v=1,3 do
			local prog = self.wanting / (km.steps-1) -- Remember: counts *down*
			local r = km.circlow + (km.circhigh-km.circlow)*prog
			local ang = random()*math.pi*2
			local x = X(r*sin(ang) + 0.5)
			local y = Y(r*cos(ang) + 0.5)
			p:addVertex(x,y,0)
		end
		local shape = bridge:meshFor(p)
		self:rcolor(shape)
		self.s:addChild(shape)
		
		table.insert(self.shapes,shape)
	end
end

function GeoGenerator:die()
	delete(self.s)
	self.block = nil self.label = nil self.count = nil
end

class "TileGenerator" (Generator)

function TileGenerator:TileGenerator(namer)
	self:_init(namer)
	self.s = self:basicScreen()
	
	self.size = 3
	
	self.shapes = {}
	
	Services.Renderer:setTextureFilteringMode(Renderer.TEX_FILTERING_NEAREST)
end

function TileGenerator:purge()
	for i, shape in ipairs(self.shapes) do
		self.s:removeChild(shape)
	end
	self.shapes = {}
end

function TileGenerator:maketiles(tilesize)
	if self.lastts == tilesize then return end
	
	self.tiles = {}
	for i=1,4 do
		local g = gfxcontainer(tilesize,tilesize)
		for x=0,tilesize-1 do
			for y=0,tilesize-1 do
				local q = math.floor( x*2/tilesize ) + math.floor( y*2/tilesize ) * 2 + 1
				local on = (q==3) or (q==1 and (i==1 or i==4)) or (q==4 and (i==2 or i == 4))
				if on then
					g:pxset(x,y,true)
				end
			end
		end
		table.insert(self.tiles, g)
	end
	self.lastts = tilesize
end

local c = Color(0,0,0,0)
function colorFor(n)
	c:setColor(n,n,n,1)
	return c
end
function rotate(n, by, base, mod)
	if n >= base then
		return (n-base + by)%mod + base
	end
	return n
end

function TileGenerator:build()
	self:purge()
	table.insert(self.shapes, self:background(self.s,0,0,0))
	
	local newsize = nil
	
	if not self.board then
		self.board = gfxcontainer(self.size,self.size)
		newsize = self.size
		
		local val = 1
		local incr = -1/(self.size*self.size-1)
		for y=1,self.size do
			for x=1,self.size do
				self.board:pxsetobj(x-1,y-1,colorFor( math.random() > 0.5 and 1 or 0 ) )
				val = val + incr
			end
		end
	else
		newsize = self.size * 3 -- iterate
		local newboard = gfxcontainer(newsize,newsize)
		
		local function cget(x,y)
			x = clamp(1,x,self.size)
			y = clamp(1,y,self.size)
			return self.board:pxc(x-1,y-1).g
		end

		local function cset(x,y, nx, ny, c)
			newboard:pxsetobj((x-1)*3+nx-1,(y-1)*3+ny-1, colorFor(c))
		end
		
		for x=1,self.size do
			for y=1,self.size do
				local get = cget(x,y)
				local left = cget(self.size-(x-1)+1,y)
				local right = cget(self.size-(x+1)+1,y)
				local up = cget(x,self.size - (y-1) + 1)
				local down = cget(x,self.size - (y+1) + 1)
				
				local newup = (up + get)/2
				local newdown = (down + get)/2
				local newleft = (left + get)/2
				local newright = (right + get)/2
				
				local tab = {get, newleft, newright, newup, newdown, (newup+newleft)/2, (newdown+newleft)/2, (newup+newright)/2, (newdown+newright)/2}
				local coord = {{2,2}, {1,2}, {3, 2}, {2, 1}, {2,3}, {1,1}, {1,3}, {3, 1}, {3,3}}
				
				for i=1,#tab do
					local tt = tab[i] -- tab[ rotate(i,#tab-3, 2,#tab-1) ]
					local tc = coord[ i ]
					if tt > 3/4 then tt = 1 end
					if tt < 1/4 then tt = 0 end
					cset(x,y, tc[1], tc[2], tt)
				end
			end
		end
		
		a(self.board)
		self.board = newboard
	end
	
	-- "Draw"
	local image = a(self.board:pximage())
	local shape = bridge:screenImageFromImage(image)
	shape:setPosition(0,0) -- FIXME: Why's center do that weird thing?
	shape:setPositionMode(ScreenEntity.POSITION_TOPLEFT)
	
	local scale = math.min(surface_height/newsize, surface_width/newsize)
	if scale < 1 then scale = 1 end
	shape:setScale(scale, scale)
	
	self.s:addChild(shape)
	table.insert(self.shapes, shape)
	
	if false then
		local guh = ScreenLabel(string.format("%f", c1.r+c1.g+c1.b), 20)
		guh:setColor(0.5,0.5,0.5,1)
		self.s:addChild(guh)
		table.insert(self.shapes, guh)
	end
	
	if false then
		table.insert(self.shapes, self:background(self.s,0,0,0,0.75))
	end
	
	-- iterate and possibly clear.
	if newsize then self.size = newsize end
	if self.size >= surface_width or self.size >= surface_height then
		self.wanting = -1
	end
end

function TileGenerator:die()
	delete(self.s)
	delete(self.board)
	self.block = nil self.label = nil self.count = nil
end

TileGenerator(CountNamer("pyramid")):run(km.steps)