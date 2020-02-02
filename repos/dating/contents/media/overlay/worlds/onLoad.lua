-- World generation -- on load

function make_rain()
	local g = r(gfxcontainer())
	local rl = 30
	for i=1,100 do
		local y = math.random(-rl,192+rl)
		local height = math.random(10,30)
		if y<0 then height = height + y y=0 end
		if height>=0 then
			g:pxfill(0xAAFFFFFF, math.random(0,288-1), y, 1, height)
		end
	end
	return g
end

class "RainCanvas" (LevelCanvas)

function RainCanvas:RainCanvas(spec)
	LevelCanvas.LevelCanvas(spec)
	self.rain = {make_rain(), make_rain(), make_rain(), make_rain(), make_rain()}
end

function RainCanvas:gfx()
	return self.rain[math.floor(ticks/4)%#self.rain + 1]
end

function rgbcode(r,g,b,a)
	return math.floor(a*255)*0x1000000 + math.floor(b*255)*0x10000 + math.floor(g*255)*0x100 + (r*255)
end


function TestCanvas(off)
	local g = r(gfxcontainer())
	local lc = LevelCanvas({g=g})
	off = off or 1

	function tin(x)
		return (x+1)/2
	end
	
	local e = Ent({onTick=function(self)
		g:pxfill(rgbcode(tin(sin((ticks+0)/60*math.pi*off)), tin(sin((ticks+20)/60*math.pi*off)), tin(sin((ticks+40)/60*math.pi*off)), 1))
	end}):insert()
	e:onTick()
	
	return lc
end

function still_star()
	local h = 4*32
	local g = r(gfxcontainer(288, h))
	g:pxfill(0xFF000000)
	for y=0,h-1 do
		for k=1,(h-y)/16 do
			local x = math.random(0,281)
			local brightness = math.random()*0.75+0.25
			g:pxfill(rgbcode(brightness,brightness,brightness,1), x,y, 1,1)
		end
	end
	return g
end

class "StarCanvas" (LevelCanvas)

function StarCanvas:StarCanvas(spec)
	LevelCanvas.LevelCanvas(spec)
	self.tileheight = spec.height or 6
	self.g = r(gfxcontainer(288, self.tileheight*32))
	self.gtemp = r(gfxcontainer(288,32))
	self.g:pxfill(0xFF000000)
	for i=1,288*self.tileheight do self:star(0,0,288,self.tileheight*32) end
	
	local upself = self
	self.ent = Ent({onTick=function(self) upself:onTick() end}):insert()
end

local minspeed,maxspeed=1,4

function StarCanvas:star(bx,by,dx,dy)
	local brightness = math.random()*0.75+0.25
	local x,y=math.random(0,dx-1),math.random(0,dy-1)
	self.g:pxfill(rgbcode(brightness,brightness,brightness,1), bx+x,by+y, 1,1)
end

function StarCanvas:onTick()
	local stepspeed = (maxspeed-minspeed)/self.tileheight
	for i=0,self.tileheight-1 do
		local at = self.g.h - (i+1)*32
--		local speed = math.pow(2,i)
		local speed = math.floor(minspeed+stepspeed*(i+1))
		self.gtemp:pxcopy(self.g, 0,0, 0,at, 288,32)
		self.g:pxcopy(self.gtemp, 0,at, speed,0, 288-speed,32)
		self.g:pxfill(0xFF000000, 288-speed, at, speed, 32)
		for k=0,i do
			self:star(288-speed,at,speed,32)
		end
	end
end

class "VortexCanvas" (LevelCanvas)

function VortexCanvas:VortexCanvas(spec)
	LevelCanvas.LevelCanvas(spec)
	self.tileheight = spec.height or 6
	self.g = r(gfxcontainer(288, self.tileheight*32))
	self.g:pxfill(0xFF0000FF)
	
	local upself = self
	self.ent = Ent({onTick=function(self) upself:onTick() end}):insert()
end

function VortexCanvas:star(bx,by,dx,dy)
	local rc = math.random() local rr = math.random()/4
	local t = math.random()*math.pi*2 local dt = math.random()*math.pi/2
	local p1 = P(rc - rr, t)
	local p2 = P(rc + rr, t+dt)
	p1 = p1:p2c(bx,by,dx,dy) p2 = p2:p2c(bx,by,dx,dy)
	
	self.g:pxline(rgbcode(math.random(),math.random(),math.random(),1), p1.x,p1.y, p2.x,p2.y)
end

function VortexCanvas:onTick()
	for i=1,3 do
		self:star(0,0,self.g.w,self.g.h)
	end
	self.g:pxfill_blend(0x08FFFFFF)
end
