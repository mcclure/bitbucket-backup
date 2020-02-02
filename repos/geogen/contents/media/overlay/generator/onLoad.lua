-- Image generation utility classes

function X(x)
	return x*surface_width
end
function Y(x)
	return x*surface_height
end

class "Namer"

function Namer:Namer(dir)
	self:setDirFromRaw(dir)
end

function Namer:setDirFromRaw(dir)
	self.dir = bridge:pwd()
	if dir then self.dir = self.dir..dir.."/" end
	if #self.dir then Services.Core:createFolder(self.dir) end
end

function Namer:next()
	return self.dir .. "default.png"
end

class "ConstNamer" (Namer)

function ConstNamer:ConstNamer(dir, name)
	self:setDirFromRaw(dir)
	self.name = name
end

function ConstNamer:next()
	return self.dir .. self.name
end

class "CountNamer" (Namer)

function CountNamer:CountNamer(dir, base)
	self:setDirFromRaw(dir) -- super
	self.i = base or 1
end

function CountNamer:next()
	local i = self.i
	self.i = self.i + 1
	return string.format("%s%d.png", self.dir, i)
end

class "DateNamer" (Namer)

-- Of course I don't support leap years
local dpm = {31,28,31,30,31,30,31,31,30,31,30,31}
function DateNamer:DateNamer(dir, y,m,d)
	self:setDirFromRaw(dir)
	self.m = m self.d = d self.y = y
end

function DateNamer:next()
	local s = string.format("%s%d-%02d-%02d.png", self.dir, self.y, self.m, self.d)
	self.d = self.d + 1
	if self.d > dpm[self.m] then
		self.d = 1
		self.m = self.m + 1
		if self.m > 12 then
			self.m = 1
			self.y = self.y + 1
		end
	end
	return s
end

-- Don't make files.
class "SwallowNamer" (Namer)

function SwallowNamer:SwallowNamer()
end

function SwallowNamer:next()
	return nil
end

class "Generator"

function Generator:Generator(namer)
	self:_init(namer)
end

function Generator:_init(namer)
	self.namer = namer or CountNamer()
end

function Generator:run(count)
	self.built = nil
	self.wanting = count
	self:insert()
end

function Generator:insert()
	if g then g:die() end
	g = self
end

function Generator:build()
end

function Generator:emit(name)
	if not name then return end
	print("" .. ticks .. " " .. name)
	self:save(name)
end

function Generator:save(name)
	local i = a(Services.Renderer:renderScreenToImage())
	i:savePNG(name)
end

function Generator:tick()
	if self.wanting then
		if self.built then
			self:emit(self.namer:next())
		end
		if self.wanting <= 0 then
			self.wanting = nil
			self:die()
			if g == self then g = nil end
		else
			self.wanting = self.wanting - 1
			self:build()
			self.built = true
		end
	end
end

function Generator:basicScreen()
	local s = Screen()
	s.ownsChildren = true
	s:setNormalizedCoordinates(false)
	return s
end

function Generator:background(s,r,g,b,a)
	r,g,b,a = r or 0, g or 0, b or 0, a or 1
	local shape = ScreenShape(ScreenShape.SHAPE_RECT, surface_width, surface_height)
	shape:setPosition(X(0.5), Y(0.5), 0)
	shape:setColor(r,g,b,a)
	s:addChild(shape)
	return shape
end

function Generator:die()
end