-- Pregame, On Load

function www(s)
	return um.base .. s;
end

userSettings = bridge:userSettings() or {}
halted = false
km = {version = 0.1,}
gm = {}
act = Queue()

function go(s)
	local fullname = string.format("media/overlay/startup\nmedia/overlay/pregame\nmedia/overlay/%s\nmedia/overlay/game\nmedia/overlay/shutdown", s)
	bridge:load_room(fullname)
end

function webTest()
	d = PolycodeDownloader("http://unicodesnowmanforyou.com")
	d:runThread()
	v = d:getDataAsString()
	print(v)
end

function webString(s,put)
	local d = a(PolycodeDownloader(s))
	if put then d:putTable("twhy", put) end
	d:runThread()
	return d:getDataAsString()
end

function webTable(s,put)
	local w = webString(s,put)
	if sillyDebug then print(w) end
	return bridge:loadTableFromXml(w)
end

function please(f)
	act:push({this=f})
end

function xhr(url, success, put)
	please(function()
		local t = webTable(url,put)
		if t then
			success(t)
		else dosFail()
		end
	end)
end

class "Spec"
function Spec:Spec(t)
	pull(self,{xdim=t.mapx,ydim=t.mapy,e={},gid=t.gid,sid=t.sid}) -- TODO: what if not xdim or not ydim
	self.scale = 10.0--surface_height/(self.ydim+2)
	self.board = {}
	local board2 = {}
	
	for k,v in ipairs(t.e) do
		if v.x and v.y then
			if v.c == 1 then
				if not self.px then
					self.px = v.x
					self.py = v.y
				end
			else
				table.insert(self.e, {x=v.x,y=v.y,c=v.c})
			end
		end
	end
	
	do -- load up board
		local x,y = 1,1
		local line = {}
		local line2 = {}
		table.insert(self.board, line)
		table.insert(board2, line2)
		for c in t.map:gmatch(".") do
			v = 0
			if c == "b" then v = 1 end
			if c == "c" then v = 2 end
			table.insert(line, v)
			table.insert(line2, v)
			
			x = x + 1
			if x > self.xdim then 
				x = 1
				line = {}
				line2 = {}
				table.insert(self.board, line)
				table.insert(board2, line2)
			end
		end
	end
	self.blocks = {} -- Parse blocks. Note. Trusts xdim, ydim! Unsafe.
	for y,line in ipairs(board2) do
		for x,c in ipairs(line) do
			if c ~= 0 then
				local bh,bw,rectangular = 1, 0, true
				for x2 = x, self.xdim do -- How wide?
					if board2[y][x2] ~= c then break end
					bw = bw + 1
				end
				for y2 = (y+1),self.ydim do
					for x2 = x,(x+bw-1) do
						if board2[y2][x2] ~= c then
							rectangular = false
							break
						end
					end
					if rectangular then bh = bh + 1 else break end
				end
				table.insert(self.blocks, {x=x,y=y,w=bw,h=bh,c=c})
				for y2 = y,(y+bh-1) do
					for x2 = x,(x+bw-1) do
						board2[y2][x2] = 0
					end
				end
			end
		end
	end
end
function Spec:readable()
	for y,line in ipairs(self.board) do
		str = ""
		for x,c in ipairs(line) do
			str = str .. (c == 0 and " " or ".")
		end
		print(str)
	end
end
function Spec:tf(b, center)
	b = tableCopy(b)
	local offx, offy = 0,0
	center = center and 1 or 0
	if b.x then
		b.x = offx+b.x*self.scale + center*(b.w*self.scale)/2
	end
	if b.y then
		b.y = offy+b.y*self.scale + center*(b.h*self.scale/2)
	end
	if b.w then
		b.w = b.w*self.scale
	end
	if b.h then
		b.h = b.h*self.scale
	end
	return(b)
end
function Spec:insert(s) -- Insert non-entity statics *only*
	for i,b in ipairs(self.blocks) do
		local b2 = self:tf(b, true)
		local shape = ScreenShape(SHAPE_RECT, b2.w, b2.h)
		if rainbowdebug then shape:setColor(math.random(),math.random(),math.random(),1) end
		shape:setPosition(b2.x, b2.y, 0)
		if b.c == 2 then -- LAVA
			shape:setColor(0.5,0,0,1)
			gm.what[shape.__ptr] = {e=false,c=b.c}
			table.insert(gm.lava, shape)
		end
		s:addPhysicsChild(shape, ENTITY_RECT, true, 0.25, 1, 0.25, false, true);
	end
end
function Spec:load(g)
	if not tinydebug then
		g.p:setNormalizedCoordinates(true, (self.ydim+2)*self.scale)
		g.p:setScreenOffset(((surface_width/surface_height)*self.ydim - self.xdim)*self.scale/2,0)
	end
	g.p:setGravity(Vector3(0,9.8*3,0))
	self:insert(g.p)
	do -- player
		local b2 = self:tf({w=0.9, h=0.9, x=self.px, y=self.py}, true)
		local shape = ScreenShape(SHAPE_RECT, b2.w, b2.h)
		shape:setColor(1,0,0,1)
		shape:setPosition(b2.x, b2.y, 0)
		g.p:addPhysicsChild(shape, ENTITY_RECT, false, 0.5, 2, 0.25, false, true);
		g.player = shape
	end
	for i,v in ipairs(self.e) do -- exits
		if v.c == 2 then
			local b2 = self:tf({w=1, h=1, x=v.x, y=v.y}, true)
			local shape = ScreenShape(SHAPE_RECT, b2.w, b2.h)
			shape:setColor(0,1,1,1)
			shape:setPosition(b2.x, b2.y, 0)
			g.p:addPhysicsChild(shape, ENTITY_RECT, true, 0.5, 2, 0.25, false, true);
			g.what[shape.__ptr] = {e=true, c=v.c}
			table.insert(g.exits, shape)
		end
	end
end

-- DOS

function killDos()
	if dos then dos:die() dos = nil end
end
function makeDos()
	killDos()
	dos = type_automaton()
	dos:insert()
end

function dosLoading()
	for x=0,39 do dos:set(x,23,"-")	end
	dos:set_centered(0,23,40, " Loading... ", false)
end

function dosFail()
--	for x=0,39 do for y=0,23 do dos:set(x,y,"*") end end
	dos:load_text("media/dos/failure.txt")
	halted = true
end

function fill(d, x,_y,w,h, with)
	with = string.rep(with,w)
	for y=_y,_y+h-1 do
		d:set(x,y,with)
	end
end