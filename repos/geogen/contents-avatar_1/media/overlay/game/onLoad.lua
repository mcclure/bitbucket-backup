-- On Load

km = {circhigh = 3, circlow = sqrt(2)/2, steps = 60}

-- Set up "spotlight" effect
--screen():setScreenShader("FilterMaterial")
--shader = shaderBindings(screen(), 
--	{center=Vector3(0.1,0.1,0.0), radius=0.1, intensity=100.0, aspect=(surface_width/surface_height)})

function X(x)
	return x*surface_height
end
Y = X

class "GeoGenerator" (Generator)

function GeoGenerator:GeoGenerator(namer)
	self:_init(namer)
	self.s = Screen()
	self.s.ownsChildren = true
	self.s:setNormalizedCoordinates(true)
	
	local shape = ScreenShape(SHAPE_RECT, X(1.0), Y(1.0))
	shape:setPosition(X(0.5), Y(0.5), 0)
	shape:setColor(0,0,0,1)
	self.s:addChild(shape)
	
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

GeoGenerator(DateNamer("geo", 2012, 9, 16)):run(km.steps)