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
	
	self.shapes = {}
end

function TileGenerator:purge()
	for i, shape in ipairs(self.shapes) do
		self.s:removeChild(shape)
	end
	self.shapes = {}
end

function TileGenerator:purgetiles()
	if self.tiles then release_all(self.tiles) end
	self.tiles = nil
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

function TileGenerator:build()
	self:purge()
	table.insert(self.shapes, self:background(self.s,0,0,0))
	local progress = (km.steps-self.wanting-1)/(km.steps-1)
	progress = 1 - progress progress = progress * progress progress = 1 - progress
	local fakeprogress = 1
	local tilesize = 128-fakeprogress*96
	local c1 = a(Color(1,1,1,1))
	
	c1:setColorHSV(math.random(360)-1,1,1-progress)
	
	local wob = (c1.r+c1.g+c1.b<1.6) and 1 or 0
	
	local c2 = a(Color())
	c2:setColor(wob,wob,wob,1)
	
	self:maketiles(tilesize)
	
	local gx,gy = surface_width, surface_height
	gx = math.ceil(gx/tilesize)*tilesize
	gy = math.ceil(gy/tilesize)*tilesize
	local g = a(gfxcontainer(gx,gy))
	g:pxfill(0xFFFFFFFF)
	
	for x = 0,surface_width-1,tilesize do
		for y = 0,surface_height-1,tilesize do
			g:pxcopy(self.tiles[math.random(#self.tiles)], x, y)
			table.insert(self.shapes, shape)
		end
	end
	
	local image = a(g:pximage())
	local blurAmount = tilesize - 7*tilesize/8 * fakeprogress + tilesize/8 * (1-progress)
	if blurAmount >= 1 then 
		image:fastBlur(blurAmount)
	end
	bridge:threshold(image, 0.5, c1, c2);
	local shape = bridge:screenImageFromImage(image)
	shape:setPosition( (surface_width-gx)/2, (surface_height-gy)/2 )
	self.s:addChild(shape)
	table.insert(self.shapes, shape)
	
	-- Now incorporate GeoGenerator -- TOOD Stick at 0.5 or 0.7
	local howmany = math.floor(progress*10)
	for i=1,howmany do
		local p = Polygon()
		for v=1,3 do
			local gfakeprog = 0.5 -- Remember: counts *down*
			local r = km.circlow + (km.circhigh-km.circlow)*gfakeprog
			local ang = random()*math.pi*2
			local x = X(r*sin(ang) + 0.5)
			local y = Y(r*cos(ang) + 0.5)
			p:addVertex(x,y,0)
		end
		local shape = bridge:meshFor(p)
		
		local c3 = a(Color(1,1,1,1))
		c3:Random()
		bridge:setColorObj(shape, c3)
		
		self.s:addChild(shape)
		
		table.insert(self.shapes,shape)
	end
	
	if false then
		local guh = ScreenLabel(string.format("%f", progress), 20)
		guh:setColor(0.5,0.5,0.5,1)
		self.s:addChild(guh)
		table.insert(self.shapes, guh)
	end
	
	if false then
		table.insert(self.shapes, self:background(self.s,0,0,0,0.75))
	end
end

function TileGenerator:die()
	delete(self.s)
	self:purgetiles()
	self.block = nil self.label = nil self.count = nil
end

TileGenerator(DateNamer("geo", 2012, 2, 25)):run(km.steps)