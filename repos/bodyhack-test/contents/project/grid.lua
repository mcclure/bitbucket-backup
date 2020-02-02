-- Grid management -- on load

-- A point. Should go in project_lua? Should integrate with Vector2 somehow?
class.P()

function P:_init(x,y)
	self.x = x self.y = y
end

function P:clone()
	return P(self.x,self.y)
end

function P:travel_axis(axis,mag)
	local p2 = self:clone()
	p2[axis] = p2[axis] + mag
	return p2
end

local dirs = {P(1,0),P(0,-1),P(-1,0),P(0,1)}
local semidirs = {P(1,0),P(1,-1),P(0,-1),P(-1,-1),P(-1,0),P(-1,1),P(0,1),P(1,1)}

function P:travel_dir(dir,mag)
	local by = dirs[dir]
	if mag then p2 = p2:mult(by, mag) end
	return self:add(by)
end

function P:travel_semidir(dir,mag)
	local by = semidirs[dir]
	if mag then p2 = p2:mult(by, mag) end
	return self:add(by)
end

function P:equals(p)
	return p.x == self.x and p.y == self.y
end

function P:floor()
	return P(math.floor(self.x),math.floor(self.y))
end

function P:norm() -- FIXME: Inefficient?
	local p2 = self:clone()
	if p2.x < 0 then p2.x = -1
	elseif p2.x > 0 then p2.x = 1 end
	if p2.y < 0 then p2.y = -1
	elseif p2.y > 0 then p2.y = 1 end
	return p2
end

function P:add(p)
	return P(self.x+p.x, self.y+p.y)
end

function P:mult(s)
	return P(self.x*s, self.y*s)
end

function P:subtract(p)
	return self:add(p:mult(-1))
end

function P:wrap(p)
	return P(mod1(self.x,p.x), mod1(self.y,p.y))
end

function P:within(p)
	return self.x>=1 and self.y>=1 and self.x<=p.x and self.y<=p.y
end

function P:str()
	return string.format("<P %d, %d>", self.x, self.y)
end

-- Polar to cartesian
function P:p2c(bx,by,dx,dy)
	local r,t = self.x,self.y
	return P( bx+dx/2 + dx/2 * r * math.cos(t),
			  by+dy/2 + dy/2 * r * math.sin(t) )
end

p0 = P(0,0)
function pos(t) return P(t.x,t.y) end
function size(t) return P(t.w,t.h) end
function set_pos(t,p) t.x,t.y=p.x,p.y end
function set_size(t,p) t.w,t.h=p.x,p.y end
function reverse_dir(p) p = p:norm() for i,v in ipairs(dirs) do if p:equals(v) then return p end end end
function reverse_semidir(p) p = p:norm() for i,v in ipairs(semidirs) do if p:equals(v) then return i end end end

-- A rectangle. Should go in project_lua?
class.Rect()

function Rect:_init(x1,x2,y1,y2)
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
class.SparseGrid()

function SparseGrid:_init()
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