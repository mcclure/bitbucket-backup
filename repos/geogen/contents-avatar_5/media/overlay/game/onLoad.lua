-- On Load

km = {steps = 64, circ = 3,
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
    local intprogress = km.steps-self.wanting
	local progress = intprogress/(km.steps-1)
	local fakeprogress = 1
	local image = Image("media/andi.png")
    
    local blurAmount = intprogress
    if blurAmount > 41 then blurAmount = 41 + (blurAmount - 41) * 2 end
    if blurAmount >= 1 then 
            image:fastBlur(blurAmount)
    end
    
    local ient = bridge:screenImageFromImage(image)
	
    self.s:addChild(ient)
    table.insert(self.shapes,shape)
	
	if false then
		local guh = ScreenLabel(string.format("%f", blurAmount), 20)
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
	self.block = nil self.label = nil self.count = nil
end

TileGenerator(DateNamer("geo", 2013, 7, 6)):run(km.steps)