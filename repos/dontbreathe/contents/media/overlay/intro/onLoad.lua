gm = {poff = 1}
km = {tglide = -0.02, clickticks = 120, maxobj = 4, bpm=16, typetimeout = 30,}

km.thumpat = {}
for k,v in pairs({[8]=0.5,[10]=1,[14]=2.0,[0]=1}) do
	local k2 = math.floor(k * km.clickticks/km.bpm)
	km.thumpat[k2] = v
end

function clamp(a,b,c)
	if b < a then return a end
	if b > c then return c end
	return b
end

function thresher(b)
	shader.thresh:set( clamp(0,b,1) )
end

gm.px1 = shader.px:get()
gm.py1 = shader.py:get()

function sizer(b)
	gm.poff = clamp(1,b,256)
	shader.px:set( gm.poff*gm.px1 )
	shader.py:set( gm.poff*gm.py1 )
end

thresher(1.0)

local id_generator = 1

function tableCount(t)
	local c = 0
	for k,v in pairs(t) do
		c = c + 1
	end
	return c
end

function idinsert(t, obj)
	t[id_generator] = obj
	obj.id = id_generator
	id_generator = id_generator + 1
end

function idsfrom(t)
	local i = {}
	for k,v in pairs(t) do
		table.insert(i, k)
	end
	return i
end

gm.floaters = {}
for i,v in ipairs({"label"}) do
	idinsert(gm.floaters, id(v))
end

function spawn()
	local repeats = 32
	local spec = {rad=math.random(surface_height/16, 2*surface_height/5),}
	
	while repeats > 0 do
		spec.x=math.random(spec.rad/2+1,surface_width-spec.rad/2-1)
		spec.y=math.random(spec.rad/2+1,surface_height-spec.rad/2-1)
		local safe = true
			
		for k,v in pairs(gm.floaters) do
			if v.spec then
				local xd,yd=v.spec.x-spec.x,v.spec.y-spec.y
				if math.sqrt(xd*xd+yd*yd)<v.spec.rad+spec.rad then
					safe = false
					break
				end
			end
		end
		
		if safe then repeats = 0 else
		repeats = repeats - 1 end
	end
	
	local ent = ScreenShape(ScreenShape.SHAPE_CIRCLE, spec.rad,spec.rad)
	
	ent.spec = spec
	
	ent:setPosition(spec.x, spec.y)
	
	local r,g,b = 0,0,0
	
	r,g,b = math.random(2)-1,math.random(2)-1,math.random(2)-1
	if r == 0 and g == 0 and b == 0 then
		r,g,b = 0.25,0.25,0.25
	end
	
	ent:setColor(r,g,b,1)
	
	idinsert(gm.floaters, ent)
	screen():addChild(ent)
	bridge:stackthrash(screen(), player)
end

for i=1,km.maxobj do spawn() end

function assertEntry()
	if not gm.entry then
		if not gm.fontSize then
			gm.fontsize = bridge:globalLastSize()
		end
	
		gm.entry = ScreenLabel("", gm.fontsize, "mono")
		
		if not gm.textmargin then
			gm.entry:setText("j")
			gm.textmargin = bridge:getTextHeight(gm.entry:getLabel())
			gm.entry:setText("")
		end
		
		gm.entrytext = ""
		gm.entry:setPosition(math.random(gm.textmargin, surface_width-gm.textmargin), math.random(1, surface_height-gm.textmargin))
		gm.entry:setColor(1,1,1,1)
		screen():addChild(gm.entry)
	end
end

function updateEntry(text)
	gm.entrytext = text
	gm.entry:setText(text)
	gm.entrylast = ticks
--	print(text)
end

function clearEntry()
	if gm.entrylast and gm.entrytext and #gm.entrytext then
		if ticks - gm.entrylast > km.typetimeout then
			idinsert(gm.floaters, gm.entry)
			gm.entrylast = nil
			gm.entrytext = nil
			gm.entry = nil
			return true
		end
	end
	return false
end


class "Latcher"

function Latcher:Latcher(snd, master, amin, apeak, amax, flickermod, noflicker)
	self.snd = snd self.master = master self.amin = amin self.apeak = apeak self.amax = amax
	self.flickermod = flickermod or 1 self.noflicker = noflicker
end

ticks = 0

function Latcher:go()
	local across = 1-shader.thresh:get()
	
	-- Get basevol
	local basevol = nil
	if across < self.amin or across > self.amax then
		basevol = 0
	elseif across < self.apeak then
		basevol = (across - self.amin)/(self.apeak - self.amin) * self.master
	elseif self.amax == self.apeak then
		basevol = 1
	else
		basevol = (self.amax - across)/(self.amax - self.apeak) * self.master
	end
	
	-- Note: The following bit is buggy as hell but when I tried to fix it it sounded less interesting

	local modmodvol = 1
	if not self.noflicker then
		local modpoff = clamp(0, (256-gm.poff*5), 256)
		local down = 1-(modpoff/128)
		if (not self.nextmax) or self.nextmax < ticks then
			self.frommax = ticks
			self.nextmax = ticks + math.random(10*self.flickermod,100/self.flickermod)
		end
		local modvol = (ticks-self.frommax)/(self.nextmax-self.frommax) * 2
		if modvol > 1 then modvol = 1 - modvol end
		modmodvol = (1-down) + down*modvol
--	print({(ticks-self.frommax)/(self.nextmax-self.frommax), self.frommax, ticks, self.nextmax})
--	print({self.master, ticks, self.frommax, self.nextmax, modvol})
	end
	
	self.snd:setVolume(basevol*modmodvol)

	return self
end

local latches = {}

local humcount = #au.hums
for i,v in ipairs(au.hums) do
	table.insert(latches, Latcher(v, 1-i/(humcount+1), 0,0,1):go())
	v:Play(true)
end


for i,v in ipairs(au.hiss) do
	table.insert(latches, Latcher(v, 0.5, 0,0.85,1, 2):go())
	v:Play(true)
end


for i,v in ipairs(au.static) do
	table.insert(latches, Latcher(v, 0.5, 0.75,1,1, 2):go())
	v:Play(true)
end

function updateSound()
end

function updateSoundInstant()
	for i,v in ipairs(latches) do v:go() end
end