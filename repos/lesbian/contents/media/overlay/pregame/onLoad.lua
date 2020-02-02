-- Pregame on load

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

-- An actual level specification in LSMQ.
class "LevelMap"

-- each Rooms rect has a bonus field "connect" containing a list of tuples {toRoom, fromCell, toCell, preferreddir} (TODO: so far only toRoom)
-- after construct, map has .entry and .exit fields
function LevelMap:LevelMap(spec)
	pull(self,
		{scale=1, wallpad=0.1})
	pull(self, spec)
	self.map = self.map or SparseGrid()
	self.theme = self.theme or LevelTheme({})
	self.entry = self.entry or self.map:bounds():center():floor()
end

function oky(o, d) -- Internal-- orientation key
	return o..d
end
function noto(o) -- Internal -- "opposite" orientation
	return o == "x" and "y" or "x"
end
-- Internal -- left turn right turn
function lt(o, d) -- checked, left turns flip dir on y, right turns flip dir on x
	return noto(o), o == "y" and d or -d
end
function rt(o, d)
	return noto(o), o == "x" and d or -d
end

local steppers =  { {x=0,y=-1}, {x=-1,y=0}, {x=0,y=1}, {x=1,y=0},}
local orienters = { {"y", -1},  {"x", -1},  {"y", 1},  {"x", 1}, }
local alphabet = { -- Notice-- flags which paths are OPEN!! Y- X- Y+ X+ [ Y- is UP ]
	["+"] = {true,  true,  true,  true},
	["-"] = {false, true,  false, true},
	["|"] = {true,  false, true,  false},
	[","] = {false, true,  true,  false},
	["L"] = {true,  false, false, true},
	["F"] = {false, false, true,  true},
	["J"] = {true,  true,  false, false},
	["T"] = {false, true,  true,  true},
	["^"] = {true,  true,  false, true},
	["<"] = {true,  true,  true,  false},
	[">"] = {true,  false, true,  true},
}

function a2dir(a)
	local o = orienters[a+1]
	return o[1],o[2]
end

function dir2a(o, d)
	return (o == "x" and 1 or 0) + (d > 0 and 2 or 0)
end

function LetterBlockedOut(c, axis, dir)
	return not alphabet[c][dir2a(axis,dir)+1]
end

function GridBlocked(grid, from, axis, dir)
	local here = grid:get(from.x, from.y)
	if not here or LetterBlockedOut(here, axis, dir) then
		return true
	end
	local otherfrom = from:travel(axis, dir)
	other = grid:get(otherfrom.x, otherfrom.y)
	if not other then return true end
	if LetterBlockedOut(other, axis, -dir) then
		return true
	end
end

-- Internal-- flood-fill from a starting point, building walls at edges. Assumes starting point is "real".
function LevelMap:addInternalSearch(grid, start)
	local todo = Queue() todo:push(start)
	local visited = SparseGrid()
	while todo.count > 0 do -- BFS until we can't BFS no more
		local from = todo:pop() -- "checking" point
		local cell = visited:get(from.x, from.y, {})
		
		if not cell.been then -- Cell visited? bail
			cell.been = true
			for i, x in ipairs( {{"x",1},{"y",1},{"x",-1},{"y",-1}} ) do -- Check four directions from this cell
				local axis, dir = x[1],x[2]
				local noky = oky(axis,dir)
				if not cell[noky] then -- Checked this direction before? Bail
					cell[noky] = true
					local to = from:travel(axis,dir) -- "next to check" point
					local tcell = not GridBlocked(grid, from, axis, dir) --  grid:get(to.x,to.y) -- Cell to check
					if tcell then -- Found another floor. Recurse.
						todo:push(to)
					else -- Hit a wall. Enter wallbuilder mode.
						local wallextent = Rect(from.x,from.x,from.y,from.y) -- Walk wall tracking all adjoining squares
						function wallcheck(travelaxis, traveldir)
							local wallcheck_from = from
							while true do
								wallcheck_from = wallcheck_from:travel(travelaxis,traveldir) -- Point where we stand to check for a wall
								if not grid:get(wallcheck_from.x,wallcheck_from.y) then break end -- Are we still standing on a floor? if not bail
								if not GridBlocked(grid, wallcheck_from, axis, dir) then break end -- Did we go off the end of the wall? if so bail
								local wallcheck_to = wallcheck_from:travel(axis, dir) -- Point at which we check for the wall
								wallextent:fit(wallcheck_from.x,wallcheck_from.y) -- Mark that we're still adjoining a wall
								visited:get(wallcheck_from.x,wallcheck_from.y,{})[noky] = true -- Mark that direction checked
								visited:get(wallcheck_to.x,  wallcheck_to.y,  {})[oky(axis, -dir)] = true -- Mark that direction checked
							end
						end
						wallcheck(lt(axis, dir)) -- Check to left
						wallcheck(rt(axis, dir)) -- Check to right
						
						self.theme:makeWall(wallextent, axis, dir)
					end
				end
			end
			if VERBOSE then
				local vcopy = visited:clone()
				vcopy:set(from.x, from.y, "@")
				print(vcopy:str())
			end
		end
	end
end

-- "Render" level into physics space.
function LevelMap:add() -- TODO: Scale
	local floors = self.map
	self:addInternalSearch(floors,self.entry)
	
	-- Monsters
	self.spawn = MonsterSpawner():insert()
	for i=2,lm.monstercount do -- insert will insert one monster automatically (undesirable?)
		self.spawn:spawn({nohomerow=true})
	end
	
	if km.floors then
		local floorbounds = floors:bounds():inset(-0.5)
		self.theme:makeFloor(floorbounds, cm.camera_height_mod-km.wallh/2)
		self.theme:makeFloor(floorbounds, cm.camera_height_mod+km.wallh/2, -1)
	end
	
	return self
end

class "Movable"

local movable_generator = 1

function Movable:Movable(spec)
	self:init(spec)
end

function Movable:init(spec)
	pull(self,
		{angle = 0, x = 0, y = 0, stepping = 0})
	pull(self, spec)
end

function Movable:insert(s)
	self.id = movable_generator
	movable_generator = movable_generator + 1
	self.home = self.intangible and gm.ent or gm.monster
	self.home[self.id] = self
	return self
end

function Movable:die()
	self.home[self.id] = nil
	if self.e then
		self.e:getParentEntity():removeChild(self.e)
		delete(self.e)
		self.e = nil
	end
	self.dead = true
end

function Movable:add(s) -- NOT INSERT
	self.x = self.x + s.x
	self.y = self.y + s.y
end

function Movable:tick()
	if self.act then
		self:process(self.act)
		self.stepping = self.stepping + 1
		if self.stepping >= (self.act.step or km.step) then
			local newact = self:complete(self.act)
			self.act = newact
			self.stepping = 0
		end
	end
	if self.e then
		local pos = self:pos()
		gm.m.theme:spritePos(self.e, pos)
	end
end

function Movable:progress()
	return self.stepping/(self.act.step or km.step)
end

function Movable:pos()
	local p = self:toP()
	if self.act and self.act[1] == "m" then
		p[self.act.axis] = p[self.act.axis] + self.act.dir * km.wallres * self:progress()
	end
	return p
end

function Movable:toP()
	return P(self.x, self.y)
end

function Movable:process(act)
end

function Movable:complete(act)
	if act and act[1] == "m" then
		self[act.axis] = self[act.axis] + act.dir
	end
end

function MonsterCollide(at, ignore)
	for k,v in pairs(gm.monster) do
		local check = v:toP()
		if v.id ~= ignore.id and at.x == check.x and at.y == check.y then
			return v
		end
	end
	return nil
end

class "MovablePlayer" (Movable)

function MovablePlayer:MovablePlayer(spec)
	self:init(spec)
end

function MovablePlayer:init(spec)
	pull(self, {q=Queue()})
	Movable.init(self,spec)
	pull(self, {angle=1})
end

function MovablePlayer:monsterCheck(censor)
	local touching = MonsterCollide(self:toP(), self)
	if touching then
		if touching:tied() then
			doKill(touching)
		else
			self.dead = true
			doDeath()
		end
	end
end

function MovablePlayer:tick()
	if cm.debugLook or self.dead then return end
	
	if pressed[KEY_LEFT] then self.q:push(KEY_LEFT) end
	if pressed[KEY_RIGHT] then self.q:push(KEY_RIGHT) end
	if pressed[KEY_DOWN] then self.q:push(KEY_DOWN) end
	if pressed[KEY_UP] then self.q:push(KEY_UP) end
	if not self.act then
		local n = self.q:pop()
		local non = not n
		if n == KEY_LEFT or (non and down[KEY_LEFT]) then
			self.act = {"a", dir=1}
		elseif n == KEY_RIGHT or (non and down[KEY_RIGHT]) then
			self.act = {"a", dir=-1}
		elseif n == KEY_DOWN then
			self.act = {"a", dir=2}
		elseif n == KEY_UP or (non and down[KEY_UP]) then
			local axis, dir = a2dir(gm.p.angle)
			if not GridBlocked(gm.m.map, self:toP(), axis, dir) then
				self.act = {"m", axis=axis, dir=dir}
			end
		end
	end
	
	self.lasering = nil
	if self.laser then
		scene():removeEntity(self.laser)
		delete(self.laser)
		self.laser = nil
	end
	if not (self.act and self.act[1] == "a") then -- Target enemy
		local axis, dir = a2dir(self.angle)
		local start = self:toP()
		local check = start:clone()

		self:monsterCheck()
		
		while true do
			if GridBlocked(gm.m.map, check, axis, dir) then
				break
			end
			check = check:travel(axis, dir)

			local collide = MonsterCollide(check, self)
			if collide then
				-- Move up
				local from = self:pos()
				local to = collide:pos()
				local naxis = noto(axis)
				local len = math.abs(from[axis]-to[axis])
				local laserCenter = P(0,0)
				laserCenter[axis] = from[axis] + len*dir/2
				laserCenter[naxis] = from[naxis]
				self.laser = ScenePrimitive(ScenePrimitive.TYPE_PLANE, km.shotw, len)
				if axis == "x" then self.laser:setYaw(90) end
				self.laser:setPosition(laserCenter.x, cm.camera_height_mod - km.shotdown, laserCenter.y)
				scene():addEntity(self.laser)
				collide:zap(self, axis, -dir)
		
				break
			end
		end
	end
	
	Movable.tick(self)
end

function MovablePlayer:complete(act)
	if act and act[1] == "a" then
		self.angle = (self.angle + act.dir) % 4
	end
	Movable.complete(self,act)
	if act and act[1] == "m" then -- Second check to present "ships passing in night" problem
		self:monsterCheck(true)
	end
end

function MovablePlayer:orient()
	local angle = self.angle
	if self.act and self.act[1] == "a" then
		angle = angle + self.act.dir*self:progress()
	end
	return angle
end

class "MovableMonster" (Movable)

function MovableMonster:MovableMonster(spec)
	self:init(spec)
end

function MovableMonster:init(spec)
	pull(self, {q=Queue()})
	Movable.init(self,spec)
end

function randomOrient()
	return a2dir(math.random(0,3))
end

function MovableMonster:tied()
	return self.act and self.act[1] == "z"
end

function MovableMonster:zapmoving()
	return self:tied() and ticks-self.act.zapped <= 1
end

function MovableMonster:zapprogress()
	return self.mstepping/(self.act.mstep or km.step)
end

function MovableMonster:pos()
	if self:zapmoving() then
		local p = self:toP()
		p[self.act.axis] = p[self.act.axis] + self.act.dir * km.wallres * self:zapprogress()
		return p
	end
	return Movable.pos(self)
end

function MovableMonster:tick()
	if not self.act then
		local nok = noto(self.angle)
		for i=1,16 do
			local axis, dir = randomOrient()
			if (self.angle ~= dir2a(axis,-dir)) and not GridBlocked(gm.m.map, self:toP(), axis, dir) then
				self.act = {"m", axis=axis, dir=dir}
				self.angle = dir2a(axis, dir)
				break
			end
		end
	end
	Movable.tick(self)
	if self.act and self.act[1] == "z" then
		if self:zapmoving() then
			self.stepping = 0
			self.mstepping = self.mstepping + 1
			if self.mstepping >= (self.act.mstep or km.step) then
				self[self.act.axis] = self[self.act.axis] + self.act.dir
				self.mstepping = 0
			end
		else
			-- Wiggle
		end
	end
end

function MovableMonster:zap(zapper, axis, dir)
	if not self.act or self.act[1] ~= "z" or self.act.axis ~= axis or self.act.dir ~= dir then
		self.act = {"z", step=km.step*4, mstep=km.step*2, axis=axis, dir=dir}
		self.mstepping = 0
	end
	if not self.act.zapped then
		au.rope:Play(false)
		gm.m.theme:setSprite(self.e, {sprite="media/sprite/109.png"})
	end
	
	self.act.zapped = ticks
end

function MovableMonster:complete(act)
	if act and act[1] == "z" then
		gm.m.theme:setSprite(self.e, {sprite="media/sprite/73.png"})
		au.escape:Play(false)
	end
	Movable.complete(self,act)
end

class "MonsterSpawner" (Movable)

function MonsterSpawner:MonsterSpawner(spec)
	self:init(spec)
end

function MonsterSpawner:init(spec)
	pull(self,
		{intangible = true, pop = 0})
	Movable.init(self,spec)
end

function MonsterSpawner:insert(s)
	Movable.insert(self,s)
	self.act = self:complete()
	return self
end

function MonsterSpawner:spawn(spec)
	spec = spec or {}
	if self.pop < lm.monstercount then
		au.spawn:Play()
		self.pop = self.pop + 1
	
		local room = gm.m.map:bounds()
		
		local p = nil
		for i=1,10000 do
			p = P(math.random(room.x1,room.x2),math.random(room.y1,room.y2))
			if not (spec.nohomerow and p.y == gm.m.entry.y) then break end
			if _DEBUG then print({"ILLEGAL MONSTER!", p.x, p.y}) end
		end
		
		local box = gm.m.theme:makeSprite(p, {sprite="media/sprite/73.png"})
		if _DEBUG then box.id = "monster" end
		MovableMonster({x=p.x, y=p.y, e=box}):insert()
	end
end

function MonsterSpawner:complete(s)
	self:spawn()
	return {"delay", step=km.step*8}
end

function tabs(n)
	n = n or 0 local r = ""
	for i=1,n do r = r .. "\t" end return r
end

function ExplainEntity(e,t)
	local num = e:getNumChildren()
	local r = tabs(t) .. to_string(e)

	for i=1,e:getNumChildren() do
		local child = e:getChildAtIndex(i-1)
		r = r .. "\n" .. ExplainEntity(child, (t or 0)+1)
	end
	return r
end

function ExplainScene(s)
	local num = s:getNumEntities()
	local r = to_string(s)
	
	for i=1,s:getNumEntities() do
		local e = s:getEntity(i-1)
		r = r .. "\n" .. ExplainEntity(e,1)
	end
	return r
end