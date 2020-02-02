-- Additional generators. Build level maps

class.ImageGenerator(Generator)

function ImageGenerator:_init(spec) self:super(spec) end

function ImageGenerator:build(w,h)
	return MapLoad(self.path)
end

-- RANDOM CRUD
function truerandom() return engine.color_make(math.random(0,255),math.random(0,255),math.random(0,255)) end
function bitrandom() return math.random()>0.5 and engine.black or engine.white end
function truebitrandom() return engine.color_make(math.random()>0.5 and 0 or 255,math.random()>0.5 and 0 or 255,math.random()>0.5 and 0 or 255) end
function random_override(self,x,y,c)
	if y ~= self.lasty then -- Much magic knowledge about loop here
		self.target = engine.channelname[math.random(3)]
		if math.random()>0.5 then
			self.yes = 0xFF self.no = 0x00
		else
			self.yes = 0x00 self.no = 0xFF
		end
		self.lasty = y
	end
	return c==self.target and self.yes or self.no
end
function bw_random_override(self,x,y,c)
	if y ~= self.lasty then -- Much magic knowledge about loop here
		if math.random()>0.5 then
			self.yes = 0xFF self.no = 0x00
		else
			self.yes = 0x00 self.no = 0xFF
		end
		self.lasty = y
	end
	return self.yes or self.no
end

function thresh_cset(self, board, fx,fy, nx, ny, channel, color)
	if color > self.thresh then color = 255 else color = 0 end
	PyramidGenerator.cset(self,board,fx,fy,nx,ny,channel,color)
end
function thresh2_cset(self, board, fx,fy, nx, ny, channel, color)
	if color < self.thresh then color = 0 end
	if color > self.thresh2 then color = 255 end
	PyramidGenerator.cset(self,board,fx,fy,nx,ny,channel,color)
end
function anti_cget(self, x, xd, y, yd, channel)
	if xd ~= 0 then x = self.size - (x+xd) + 1 end
	if yd ~= 0 then y = self.size - (y+xd) + 1 end
	x = engine.clamp(1,x,self.size)
	y = engine.clamp(1,y,self.size)
	return project.get_color_x(self.state,channel,x-1,y-1)
end


require("project/g_subdivide")

generators = {
	PyramidGenerator({thresh=255/2, cset=thresh_cset, cget=anti_cget}),
--[[
	-- 2. Still (1)
	ImageGenerator({path="resource/stage/scribble2.png"}),
	
	-- 3. Static (2)
	Generator({build=function(self,w,h) return MapRandom(w,h, PyramidGenerator.random) end}),
	
	-- 4. Neocities (3)
	PyramidGenerator(),
	PyramidGenerator({random=truerandom}),
--	PyramidGenerator({random=truebitrandom}),
	PyramidGenerator({mutate_tab=function(self,i,tabcount) return rotate(i,math.random(0,3)*2, 2,tabcount-1) end}),
	PyramidGenerator({mutate_tab=function(self,i,tabcount,channel) return rotate(i,channel+1, 2,tabcount-1) end}),
	PyramidGenerator({centerget=truerandom}),
	PyramidGenerator({centerget=truerandom,random=truerandom}),
	PyramidGenerator({centerget=bw_random_override}),
	PyramidGenerator({centerget=bw_random_override, random=truerandom}),
	
	-- STICKETS?! (13)
	PyramidGenerator({thresh=255/2, cset=thresh_cset, random=truebitrandom}),
	PyramidGenerator({thresh=255/2, cset=thresh_cset, random=truerandom}),
	
	-- Girder city (15)
	PyramidGenerator({thresh=255/2, cset=thresh_cset, random=truerandom, cget=anti_cget}),
--	PyramidGenerator({thresh=255/2, cset=thresh_cset, cget=anti_cget}), -- BW above
		
	-- Calmgirder (16)
	PyramidGenerator({thresh=255*1/3, thresh2=255*2/3, cset=thresh2_cset, random=truebitrandom, cget=anti_cget}),
	PyramidGenerator({thresh=255*1/3, thresh2=255*2/3, cset=thresh2_cset, random=bitrandom, cget=anti_cget}),
	
	-- Purr (19)
	PyramidGenerator({thresh=255*2/5, thresh2=255*3/5, cset=thresh2_cset, random=bitrandom}),
	PyramidGenerator({thresh=255*2/5, thresh2=255*3/5, cset=thresh2_cset, random=truebitrandom}),
--]]
}