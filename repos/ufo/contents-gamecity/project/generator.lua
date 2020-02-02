-- Additional generators. Build level maps

class.BrogGenerator(Generator)

function BrogGenerator:_init(spec) self:super(spec) end

function BrogGenerator:build(w,h)
	local map = Generator.build(self, w, h)

	-- fill in level array
	project.level_init(map, self.worms, self.worm_length, self.worm_start_dist, self.col_brig, self.col_dark, self.col_bg)

	return map
end

class.BrogGenerator2(Generator)

function BrogGenerator2:_init(spec) self:super(spec) end

function BrogGenerator2:build(w,h)
	local map = Generator.build(self, w, h)

	-- fill in level array
	project.level_init2(map, self.worms, self.worm_length, self.worm_start_dist, self.col_brig, self.col_dark)

	return map
end

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
	--ImageGenerator({path="resource/stage/lesson_3.png"}),
	--ImageGenerator({path="resource/stage/screensh.png"}),

	-- 1. Brog
	BrogGenerator2({col_brig=engine.color_make(0xff,0x20,0x55),col_dark=engine.color_make(0xff,0x99,0x99),
		worms=100,worm_length=1,worm_start_dist=10}),
	BrogGenerator2({col_brig=engine.color_make(0x10,0x20,0x55),col_dark=engine.color_make(0xbb,0x88,0x00),
		worms=100,worm_length=3,worm_start_dist=16}),
	BrogGenerator2({col_brig=engine.color_make(0xff,0xff,0xff),col_dark=engine.color_make(0x33,0x66,0x99),
		worms=4,worm_length=128,worm_start_dist=0}),
	BrogGenerator2({col_brig=engine.color_make(0x33,0x99,0x99),col_dark=engine.color_make(0x00,0xff,0x00),
		worms=100,worm_length=3,worm_start_dist=8}),

	BrogGenerator({col_brig=engine.color_make(0x77,0x44,0x44),col_dark=engine.color_make(0xee,0xaa,0xaa),col_bg=engine.color_make(0x00,0x00,0x00),
		worms=1,worm_length=8,worm_start_dist=0}),
	BrogGenerator({col_brig=engine.color_make(0x22,0x99,0x77),col_dark=engine.color_make(0x66,0xcc,0x33),col_bg=engine.color_make(0xff,0x00,0x00),
		worms=100,worm_length=3,worm_start_dist=8}),
	BrogGenerator({col_brig=engine.color_make(0x00,0x00,0x00),col_dark=engine.color_make(0x70,0x90,0xff),col_bg=engine.color_make(0x60,0x40,0x00),
		worms=8,worm_length=8,worm_start_dist=4}),
	BrogGenerator({col_brig=engine.color_make(0xff,0xff,0xff),col_dark=engine.color_make(0x00,0xff,0xff),col_bg=engine.color_make(0xff,0x00,0xff),
		worms=100,worm_length=4,worm_start_dist=32}),
	BrogGenerator({col_brig=engine.color_make(0x00,0xAA,0x00),col_dark=engine.color_make(0x00,0x00,0x00),col_bg=engine.color_make(0x00,0xff,0x99),
		worms=24,worm_length=3,worm_start_dist=16}),
	BrogGenerator({col_brig=engine.color_make(0x00,0x00,0x00),col_dark=engine.color_make(0xff,0xff,0x00),col_bg=engine.color_make(0x00,0x00,0x00),
		worms=4,worm_length=20,worm_start_dist=16}),
	BrogGenerator({col_brig=engine.color_make(0xCC,0xAA,0xCC),col_dark=engine.color_make(0x77,0x33,0x88),col_bg=engine.color_make(0x00,0x00,0x00),
		worms=3,worm_length=6,worm_start_dist=0}),
	BrogGenerator({col_brig=engine.color_make(0xFF,0xFF,0xFF),col_dark=engine.color_make(0xBB,0x99,0x55),col_bg=engine.color_make(0x00,0x00,0x00),
		worms=16,worm_length=1,worm_start_dist=16}),
	BrogGenerator({col_brig=engine.color_make(0x00,0x00,0x00),col_dark=engine.color_make(0x50,0x50,0x75),col_bg=engine.color_make(0xe0,0xe0,0xff),
		worms=16,worm_length=128,worm_start_dist=0}),
		
	-- 2. Still (14)
	ImageGenerator({path="resource/stage/scribble2.png"}),
	
	-- 3. Static (15)
	Generator({build=function(self,w,h) return MapRandom(w,h, PyramidGenerator.random) end}),
	
	-- 4. Neocities (16)
	PyramidGenerator(),
	PyramidGenerator({random=truerandom}),
--	PyramidGenerator({random=truebitrandom}),
	PyramidGenerator({mutate_tab=function(self,i,tabcount) return rotate(i,math.random(0,3)*2, 2,tabcount-1) end}),
	PyramidGenerator({mutate_tab=function(self,i,tabcount,channel) return rotate(i,channel+1, 2,tabcount-1) end}),
	PyramidGenerator({centerget=truerandom}),
	PyramidGenerator({centerget=truerandom,random=truerandom}),
	PyramidGenerator({centerget=bw_random_override}),
	PyramidGenerator({centerget=bw_random_override, random=truerandom}),
	
	-- STICKETS?! (24)
	PyramidGenerator({thresh=255/2, cset=thresh_cset, random=truebitrandom}),
	PyramidGenerator({thresh=255/2, cset=thresh_cset, random=truerandom}),
	
	-- Girder city (27)
	PyramidGenerator({thresh=255/2, cset=thresh_cset, random=truerandom, cget=anti_cget}),
--	PyramidGenerator({thresh=255/2, cset=thresh_cset, cget=anti_cget}), -- BW above
		
	-- Calmgirder (29)
	PyramidGenerator({thresh=255*1/3, thresh2=255*2/3, cset=thresh2_cset, random=truebitrandom, cget=anti_cget}),
	PyramidGenerator({thresh=255*1/3, thresh2=255*2/3, cset=thresh2_cset, random=bitrandom, cget=anti_cget}),
	
	-- Purr (31)
	PyramidGenerator({thresh=255*2/5, thresh2=255*3/5, cset=thresh2_cset, random=bitrandom}),
	PyramidGenerator({thresh=255*2/5, thresh2=255*3/5, cset=thresh2_cset, random=truebitrandom}),
}