class.IterGenerator(Generator)

function IterGenerator:_init(spec) self:super(spec) end

function IterGenerator:initial()
	return MapLoad(self.path)
end

function IterGenerator:build(w,h)
	local result = nil
	self.state = self:initial()
	while not result do
		result = self:pass(w,h)
	end
	return result
end

function MapRandom(w,h,f)
	local state = MapMake(w,h)
	for x=1,w do
		for y=1,h do
			project.set_color(state, x-1,y-1, f() )
		end
	end
	return state
end

function MapCenterCrop(map, w, h)
	if map.w == w and map.h == h then return map end
	local result = MapMake(w,h)
	engine.blit_whole(result, map, (result.w-map.w)/2, (result.h-map.h)/2)
	MapKill(map)
	return result
end

class.PyramidGenerator(IterGenerator)

function PyramidGenerator:_init(spec)
	self:super(spec)
end

function PyramidGenerator:random()
	return engine.color_gray(math.random(0,255))
end

function PyramidGenerator:initial()
	self.size = 3
	return MapRandom(self.size, self.size, self.random)
end

function PyramidGenerator:cget(x, xd, y, yd, channel)
	x = x + xd
	y = y + yd
	x = engine.clamp(1,x,self.size)
	y = engine.clamp(1,y,self.size)
	return project.get_color_x(self.state,channel,x-1,y-1)
end

function PyramidGenerator:centerget(x,y,channel)
	return self:cget(x,0,y,0,channel)
end

function rotate(n, by, base, mod)
	if n >= base then
		return (n-base + by)%mod + base
	end
	return n
end

function PyramidGenerator:mutate_tab(i,tabcount)
	return i
end

function PyramidGenerator:cset(newboard, fx,fy, nx, ny, channel, color)
	local x, y = (fx-1)*3+nx-1,(fy-1)*3+ny-1
	project.set_color_x(newboard, channel, x,y, color)
end

function PyramidGenerator:pass(w,h)
	local newsize = self.size * 3
	local newboard = MapMake(newsize, newsize)
	
	for x=1,self.size do
		for y=1,self.size do
			for c=0,2 do -- c for channel
				local get =   self:cget(x, 0,  y, 0,  c)
				local left =  self:cget(x, -1, y, 0,  c)
				local right = self:cget(x, 1,  y, 0,  c)
				local up =    self:cget(x, 0,  y, -1, c)
				local down =  self:cget(x, 0,  y, 1,  c)
				
				local newup = (up + get)/2
				local newdown = (down + get)/2
				local newleft = (left + get)/2
				local newright = (right + get)/2
				
				local tab = {self:centerget(x,y,c), newleft, newright, newup, newdown, (newup+newleft)/2, (newdown+newleft)/2, (newup+newright)/2, (newdown+newright)/2}
				local coord = {{2,2}, {1,2}, {3, 2}, {2, 1}, {2,3}, {1,1}, {1,3}, {3, 1}, {3,3}}
				
				local tabcount = #tab
				for i=1,tabcount do
					local tt = tab[ self:mutate_tab(i, tabcount, c) ]
					local tc = coord[ i ]
					self:cset(newboard, x,y, tc[1], tc[2], c, tt)
				end
			end
		end
	end
	
	MapKill(self.state)
	self.state = newboard
	self.size = newsize
	
	if self.size >= w and self.size >= h then
		return MapCenterCrop(self.state, w, h)
	end
	return nil
end

function InPlacePyramid(to, from, inmap) -- ASSUME SQUARE. THIS IS NOT THE BEST SOFTWARE ENGINEERING PRACTICES I'VE EVER SEEN
	local size=to.w

	local function cget(x,y, channel)
		x,y = inmap(size,x,y)
		return project.get_color_x(from,channel,x-1,y-1)
	end

	local function cset(fx,fy, nx, ny, channel, color)
		local x, y = (fx-1)*3+nx,(fy-1)*3+ny
		x = engine.clamp(1,x,size)
		y = engine.clamp(1,y,size)
		project.set_color_x(to, channel, x-1,y-1, color)
	end
	
	for x=1,math.ceil(size/3) do
		for y=1,math.ceil(size/3) do
			for c=0,2 do -- c for channel
				local get = cget(x,y,c)
				local left = cget(x-1,y,c)
				local right = cget(x+1,y,c)
				local up = cget(x,y-1,c)
				local down = cget(x,y+1,c)
				
				local newup = (up + get)/2
				local newdown = (down + get)/2
				local newleft = (left + get)/2
				local newright = (right + get)/2
				
				local tab = {get, newleft, newright, newup, newdown, (newup+newleft)/2, (newdown+newleft)/2, (newup+newright)/2, (newdown+newright)/2}
				local coord = {{2,2}, {1,2}, {3, 2}, {2, 1}, {2,3}, {1,1}, {1,3}, {3, 1}, {3,3}}
				
				local tabcount = #tab
				for i=1,tabcount do
					local tt = tab[ i ]
					local tc = coord[ i ]
					cset(x,y, tc[1], tc[2], c, tt)
				end
			end
		end
	end
end