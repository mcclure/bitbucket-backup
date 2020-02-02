-- On Load

km = {steps = 65, circ = 3,
	gray = {low=0,high=0.5},
	bw   = {low=0.4,high=1.0},
	stat = {low=0.65,high=1}
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
			local x = r*sin(ang) + 0.5
			local y = r*cos(ang) + 0.5
			p:addVertex(X(x),Y(y),0,x,y)
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

local pen = r(Color(0,0,0,0))

function stair(p, pfrom,pto, vfrom,vto)
	vfrom = vfrom or 0
	vto = vto or 1
	return signedclamp(vfrom, vfrom + (p-pfrom) / (pto-pfrom) * (vto-vfrom), vto)
end

function ease2(progress)
	return progress*progress*progress
end

function ease3(progress)
	return progress*progress*progress
end

function unease(progress)
	progress = 1 - progress progress = progress * progress progress = 1 - progress
	return progress
end

function TileGenerator:build()
	self:purge() -- clear out last frame
	table.insert(self.shapes, self:background(self.s,0,0,0))
	local progress = (km.steps-self.wanting-1)/(km.steps-1)
	local fakeprogress = 1
	local c1 = a(Color(1,1,1,1))
	
	-- Make bitmap space
	local gx,gy = surface_width, surface_height
	local g = a(gfxcontainer(gx,gy))
	g:pxfill(0xFFFFFFFF)
	
	-- Make bitmap
	local dx = 8 local dy = 4
	for _y = 0,surface_height-1,dy do
		for _x = 0,surface_width-1,dx do
			local gray = math.random()
			pen:setColor(gray, gray, gray, 1)
			for y = _y,_y+dy-1 do
				for x = _x,_x+dx-1 do
					g:pxsetobj(x,y,pen)
				end
			end
		end
	end
	
	local image = a(g:pximage())
	local texture = nil
	
	-- Now incorporate GeoGenerator
	local howmany = 30
	for i=1,howmany do
		local p = Polygon()
		for v=1,3 do
			local gfakeprog = 0.5 -- Remember: counts *down*
			local r = km.circ
			local ang = random()*math.pi*2
			local x = r*sin(ang) + 0.5
			local y = r*cos(ang) + 0.5
			p:addVertex(X(x),Y(y),0,x,y)
		end
		
		local c3 = a(Color(1,1,1,1))
		local shape = bridge:meshFor(p)
		
		if math.random() > ( stair(progress, km.stat.low, km.stat.high,1.0,0.0) ) then -- color or texture?
			if texture then
				shape:setTexture(texture)
			else
				bridge:loadTextureFromImage(shape,image)
				texture = shape:getTexture()
			end
		else
			c3:Random()
			
			local hue = c3:getHue()
			local sat = c3:getSaturation()
			local val = c3:getValue()
			sat = sat * ease3( stair(progress, km.gray.low, km.gray.high, 1.0, 0.0) )
			local bwmod = ease3( stair(progress, km.bw.low, km.bw.high, 1.0, 0.0) )
			if math.random() > bwmod then
				val = val > 0.7 and 1.0 or 0.0
			end
			
			c3:setColorHSV(hue, sat, val)
		end
		
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
	
	if true then
		table.insert(self.shapes, self:background(self.s,0,0,0,0.75))
	end
end

function TileGenerator:die()
	delete(self.s)
	self.block = nil self.label = nil self.count = nil
end

TileGenerator(DateNamer("geo", 2013, 5, 1)):run(km.steps)