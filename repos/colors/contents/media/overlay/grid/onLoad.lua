-- Grid management -- on load

-- A point. Should go in project_lua? Should integrate with Vector2 somehow?
class "P"

function P:P(x,y)
	self.x = x self.y = y
end

function P:clone()
	return P(self.x,self.y)
end

function P:travel(axis,dir)
	local p2 = self:clone()
	p2[axis] = p2[axis] + dir
	return p2
end

function P:equals(p)
	return p.x == self.x and p.y == self.y
end

function P:floor()
	return P(math.floor(self.x),math.floor(self.y))
end

function P:add(p)
	return P(self.x+p.x, self.y+p.y)
end

function P:mult(s)
	return P(self.x*s, self.y*s)
end

function P:str()
	return string.format("<P %d, %d>", self.x, self.y)
end

p0 = P(0,0)

-- A rectangle. Should go in project_lua?
class "Rect"

function Rect:Rect(x1,x2,y1,y2)
	pull(self, {x1=x1 or 0, x2=x2 or 0, y1=y1 or 0, y2=y2 or 0})
	if self.x2 < self.x1 then local temp = self.x1 self.x1 = self.x2 self.x2 = temp end
	if self.y2 < self.y1 then local temp = self.y1 self.y1 = self.y2 self.y2 = temp end
end

function Rect:center()
	return P( (self.x1 + self.x2) / 2, (self.y1 + self.y2) / 2 )
end

-- Expand rectangle to contain point
function Rect:fit(x,y)
	if x < self.x1 then self.x1 = x end
	if x > self.x2 then self.x2 = x end
	if y < self.y1 then self.y1 = y end
	if y > self.y2 then self.y2 = y end
	return self
end

-- Expand or contract boundaries by margins
function Rect:inset(x,y)
	y = y or x
	self.x1 = self.x1 + x
	self.x2 = self.x2 - x
	self.y1 = self.y1 + y
	self.y2 = self.y2 - y
	return self
end

function Rect:offset(x,y)
	self.x1 = self.x1 + x
	self.x2 = self.x2 + x
	self.y1 = self.y1 + y
	self.y2 = self.y2 + y
end

-- Test if point is in rect
function Rect:collide(x,y)
	return (self.x1 <= x and self.x2 >= x) and (self.y1 <= y and self.y2 >= y)
end

function Rect:rectCollideInternal(r)
	return self:collide(r.x1,r.y1) or self:collide(r.x2,r.y1) or self:collide(r.x1,r.y2) or self:collide(r.x2,r.y2)
end

-- Test if rect overlaps with other rect
function Rect:rectCollide(r)
	return self:rectCollideInternal(r) or r:rectCollideInternal(self)
end

-- Clone
function Rect:clone()
	return Rect(self.x1, self.x2, self.y1, self.y2)
end

-- A sparse 2D grid. Should go in project_lua?
class "SparseGrid"

function SparseGrid:SparseGrid()
	self.data = {}
end

function SparseGrid:set(x, y, value)
	if not self.data[x] then self.data[x] = {} end
	self.data[x][y] = value
end
function SparseGrid:get(x, y, default)
	local xv = self.data[x]
	local yv = xv and xv[y]
	if not yv and default then self:set(x,y,default) return default end
	return yv
end

-- Returns an iterator that returns P(x,y),value for every point in grid
function SparseGrid:iter()
	local ix, x = nil, nil
	local iy, y = nil, nil
	return function()
		if x then iy, y = next(x, iy) end
		while not iy do 
			ix, x = next(self.data, ix)
			if not ix then return nil end
			iy, y = next(x)
		end
		return P(ix, iy), y
	end
end

-- Returns Rect covering all existing points in grid
function SparseGrid:bounds()
	local r = nil
	for k,v in self:iter() do
		local x = k.x local y = k.y
		if not r then r = Rect(x,x,y,y) else r:fit(x,y) end
	end
	return r
end

-- Sparse grid clone
function SparseGrid:clone()
	local r = SparseGrid()
	for k,v in self:iter() do
		local x = k.x local y = k.y
		r:set(x,y,v)
	end
	return r
end

-- Returns string w/all grid values -- "." for empty -- assumes values are 1-char strings
function SparseGrid:str()
	local bounds = self:bounds()
	local r = "\n"
	for y = bounds.y1, bounds.y2 do
		for x = bounds.x1, bounds.x2 do
			local cell = self:get(x,y)
			local ct = type(cell)
			if ct == "table" then cell = "-"
			elseif cell and ct ~= "string" then cell = tostring(cell) end
			r = r .. (cell or ".")
		end
		r = r .. "\n"
	end
	return r
end

function GridFrom(path)
	local from = bridge:filedump(path)
	local x,y = 0,1
	local g = SparseGrid()
	for c in from:gmatch(".") do
		x = x + 1
		if c == "\n" then
			y = y + 1
			x = 0
		elseif c ~= " " then
			g:set(x,y,c)
		end
	end
	return g
end