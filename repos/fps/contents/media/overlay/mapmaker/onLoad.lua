-- Map generation tools

function pull(dst, src) -- Used lots of places. Should go in project_lua? Is there a std function for this?
	if dst and src then
		for k,v in pairs(src) do
			dst[k] = v
		end
	end
end

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

-- Returns string w/all grid values -- "." for empty -- assumes values are 1-char strings
function SparseGrid:str()
	local bounds = self:bounds()
	local r = "\n"
	for y = bounds.y1, bounds.y2 do
		for x = bounds.x1, bounds.x2 do
			local cell = self:get(x,y)
			r = r .. (cell or ".")
		end
		r = r .. "\n"
	end
	return r
end

-- An actual level specification in 7DRL.
class "LevelMap"

function fT(tn)
	force_theme_num = tn
	gm = nil
	bridge:rebirth()
end
function tS(s) -- theme sum
	local r = 0
	for i,t in ipairs(s) do r = r + (t.w or 1) end
	return r
end
function rT(s)
	local sum = tS(s)
	local target = math.random()*sum
	local r = 0
	for i,t in ipairs(s) do
		r = r + (t.w or 1)
		if r > target then return i end
	end
	return s[0] -- Should be impossible
end

-- each Rooms rect has a bonus field "connect" containing a list of tuples {toRoom, fromCell, toCell, preferreddir} (TODO: so far only toRoom)
-- after construct, map has .entry and .exit fields
function LevelMap:LevelMap(spec)
	pull(self,
		{xs = 30, ys = 30, lrs = 2, rs = 10, m = 3, roomcount=4, pushaway=false, scale=1, wallpad=0.1,
		 rooms = {}, monsters = SparseGrid()})
	pull(self, spec)
	if not self.theme then
		local themes = {
			{sky = { {{0,0,45/255}}, {{0,1,0},{0,90/255,0}} }},
			{sky = { {{1,1,1},{0,0,0}}, {{0,0,0},{1,1,1}} }},
			{sky = { {{0.5,0,0},{0,0,0}}, {{0,0,0},{0.5,0,0}} },
				wallcolor=function(w) w:setColor(0.5,0,0,1) end,
				randomBoxColor=function(s,b) b.r = (1+math.random())/2 b.g = (1+math.random())/2 b.b = (1+math.random())/2 end
			},
			{w=2},
			{sky = { {{1,0,0}} }, flat={0,0,0}, w=1/3},
			{sky = { {{0,0.5,0}} }, flat={0,0,0}, w=1/3},
			{sky = { {{0,0,1}} }, flat={0,0,0}, w=1/3},
		}
		local themepick = 1
		if force_theme_num then themepick = force_theme_num
		elseif gm.at_level > 1 then themepick = rT(themes) end
		
		self.theme = LevelTheme(themes[themepick])
	end
end

-- Call connected() with no args, will return true if graph connected
function LevelMap:connected(startat, visited, visitedcount)
	if not startat then 
		startat = self.rooms[1]
		if not startat then return true end
		visited = {}
		visitedcount = 0
	end
	
	for i,tuple in ipairs(startat.connect) do
		local room = tuple[1]
		if not visited[room] then
			visited[room] = true
			visitedcount = visitedcount + 1
			if visitedcount >= #self.rooms then return true end
			if self:connected(room, visited, visitedcount) then return true end
		end
	end
	return false
end

-- Return a random point within some room
function LevelMap:randomWithin()
	local rnum = math.random(self.roomcount)
	local room = self.rooms[rnum]
	return {r=rnum, at=P(math.random(room.x1,room.x2),math.random(room.y1,room.y2))}
end
-- Return a random point with particular properties: Not this point, optionally this room
function LevelMap:randomWithinNotMatching(badat, r, badr)
	local result
	for x=1,100 do
		result = self:randomWithin()
		if (not r or r == result.r) and (not badr or badr ~= result.r) and not result.at:equals(badat) then break end
	end
	return result
end

-- Given a RandomWithin() result, return the angle from there to room center.
function LevelMap:center_angle_for(data)
	return vArgXy(vSub(data.at,self.rooms[data.r]:center()))/math.pi*180
end

-- Generate a random map
function LevelMap:construct()
	for r = 1,self.roomcount do --  Build rooms themselves
		local x = math.random(self.xs) local y = math.random(self.ys) local xd = math.random(self.lrs,self.rs) local yd = math.random(self.lrs,self.rs)
		local r = Rect(x, x+xd, y, y+yd)
		r.connect = {}
		table.insert(self.rooms, r)
	end
	
	self.entry = self:randomWithin()
	self.exit = self:randomWithinNotMatching(self.entry.at, nil, self.entry.r)
	
	for r = 1,self.roomcount do --  Populate with monsters
		if r ~= self.entry.r then -- No monsters in starting room
			local monstercount = math.random(1,self.m) -- At least one monster
			for i = 1,monstercount do
				local p = self:randomWithinNotMatching(self.entry.at, r)
				self.monsters:set(p.at.x,p.at.y,monsters[math.random(#monsters)])
			end
		end
	end
	
	while not self:connected() do -- Add connections between rooms
		local from = math.random(self.roomcount)
		local to = math.random(self.roomcount)
		if from ~= to then -- TODO: Test for double connection 
			--if _DEBUG then print(string.format("Adding from %d to %d", from, to)) end
			table.insert( self.rooms[from].connect, {self.rooms[to]} )
		end
	end
	return self
end

function intp(t) -- Internal
	return P(math.floor(t.x),math.floor(t.y))
end

-- Returns a grid. Not necessarily for debugging.
function LevelMap:debugGrid(floor_only)
	local r = SparseGrid()
	for i,room in ipairs(self.rooms) do -- Splat all rooms
		for x = room.x1,room.x2 do for y = room.y1,room.y2 do
			r:set(x,y,"#")
		end end
		for i,connect in ipairs(room.connect) do -- Splat room connections
			local c1 = intp(room:center())
			local c2 = intp(connect[1]:center())
			local path = Rect(c1.x,c2.x,c1.y,c2.y)
			for i = path.y1,path.y2 do
				r:set(path.x1,i,"#")
				r:set(path.x2,i,"#") -- Really?
			end
			for i = path.x1,path.x2 do
				r:set(i,path.y1,"#")
				r:set(i,path.y2,"#")
			end
		end
	end
	if not floor_only then 
		for k,v in self.monsters:iter() do -- Splat all monsters
			r:set(k.x,k.y,v[1])
		end
		r:set(self.entry.at.x,self.entry.at.y,"@")
		r:set(self.exit.at.x,self.exit.at.y,"%")
	end
	return r
end

-- Print sparse grid form to stdout. For debugging.
function LevelMap:debugPrint()
	print(self:debugGrid():str())
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
					local tcell = grid:get(to.x,to.y) -- Cell to check
					if tcell then -- Found another floor. Recurse.
						todo:push(to)
					else -- Hit a wall. Enter wallbuilder mode.
						local wallextent = Rect(from.x,from.x,from.y,from.y) -- Walk wall tracking all adjoining squares
						function wallcheck(travelaxis, traveldir)
							local wallcheck_from = from
							while true do
								wallcheck_from = wallcheck_from:travel(travelaxis,traveldir) -- Point where we stand to check for a wall
								if not grid:get(wallcheck_from.x,wallcheck_from.y) then break end -- Are we still standing on a floor? if so bail
								wallcheck_to = wallcheck_from:travel(axis, dir) -- Point at which we check for the wall
								if grid:get(wallcheck_to.x,wallcheck_to.y) then break end -- Did we go off the end of the wall? if so bail
								wallextent:fit(wallcheck_from.x,wallcheck_from.y) -- Mark that we're still adjoining a wall
								visited:get(wallcheck_from.x,wallcheck_from.y,{})[noky] = true -- Mark that direction checked
							end
						end
						wallcheck(lt(axis, dir)) -- Check to left
						wallcheck(rt(axis, dir)) -- Check to right
						
						self.theme:makeWall(wallextent, axis, dir)
					end
				end
			end
		end
	end
end

-- "Render" level into physics space.
function LevelMap:add(s, enemies, revisit) -- TODO: Scale
	local floors = self:debugGrid(true)
	self:addInternalSearch(floors,self.entry.at)
	
	local entrydata = self.entry
	local exitdata = self.exit
	if revisit then local temp = entrydata entrydata = exitdata exitdata = temp end -- Swap
	
	local you = a(Vector3(entrydata.at.x,2,entrydata.at.y))
	local box = self.theme:makeBoxWithLetter(you.x, you.y, you.z, "@", 0.5)
	box:setColor(1,1,1,0)
	cm.angle = self:center_angle_for(entrydata)
	noRotation(box)
	player = box
	
	box = self.theme:makeBoxWithLetter(exitdata.at.x,1,exitdata.at.y, "%")
	box:setColor(1,1,1,1)
	noMovement(box)
	exit = box
	
	local imonsters
	if revisit then
		imonsters = SparseGrid()
		for k,v in floors:iter() do
			if math.random(4) == 1 then
				imonsters:set(k.x,k.y,monsters[math.random(#monsters)])
			end
		end
	else
		imonsters = self.monsters
	end
	
	for k,v in imonsters:iter() do
		local safe = true
		local them = a(Vector3(k.x,1,k.y))
		
		if gm.at_level <= 2 or revisit then
			local coll = a(bridge:getFirstEntityInRay(s, them, you))
			coll = coll and coll:entity()
			if coll and coll.__ptr == player.__ptr then
				safe = false
			end
		end
		
		if safe then
			local box = self.theme:makeBoxWithLetter(them.x,them.y,them.z, v[1])
			self.theme:randomBoxColor(box)
			box:setColor(box.r,box.g,box.b,1)
			noRotation(box)
			enemies[box.__ptr] = box -- todo: onClose() delete enemies
			enemycount = enemycount + 1
			box.lastAct = -1
			enemyqueue:push(box)
			box.proto = v
			box.hp = box.proto.hp
			fireplan(box)
			emoteplan(box)
		end
	end
		
	return self
end