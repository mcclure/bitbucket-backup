-- Themeing -- On load

tm = {}

fm.level = fm.level or 0

-- "DEFAULTS"

function tm.howtall()
	return fm.level == 1 and 6 or 12
end

function tm.howmany()
	return 100*clamp(1,fm.level)
end

function tm.boxposition(box, i)
	box:setPosition(randAround(0, 6, 15), randAround(0, tm.howtall()), randAround(0,6,15))
end

function tm.boxcolor(box, i)
	box:setMaterialByName("AtomMaterial")
	box:setColor(math.random(), math.random(), math.random(),1)
end

function tm.clearColor()
	Services.Renderer:setClearColor(0xDD/0xFF,0xDD/0xFF,0xDD/0xFF,1)
end

-- gfx/au

tm.gfx = {blur=.08, thresh=0.409, brighten=1.35}
tm.au = {param={92.57, 1, 1}, iparam={4, 5256}, volume=0.5}

-- Not intended for overload as such
local blurm = {blur=1, stretch=0}
function newblur(t, value)
	blurm[t] = value
	for i=1,2 do
		local v2 = 1/(16+(1-blurm.blur)*(1024-16))
		v2 = math.sqrt(v2)
		v2 = v2 - math.sqrt(1/1024)
		if i == 1 and blurm.stretch < 0 then v2 = v2 * (1+blurm.stretch) end
		if i == 2 and blurm.stretch > 0 then v2 = v2 * (1-blurm.stretch) end
		shaders[i].blurSize:set(v2)
	end
end
function rawendshader(t, value)
	shaders[2][t]:set(value)
end

class "PulseEnt" (TimedEnt) -- Crosshair UI
function PulseEnt:PulseEnt(spec)
	TimedEnt.TimedEnt(self,spec)
	self.lastMove = 0
	self.draining = 0
	self.crashed = 0
	self.crashDrain = 0
	self.s = r(Screen())
	
	local crossSize = surface_height/40
	local crossCircle = ScreenShape(ScreenShape.SHAPE_CIRCLE, crossSize, crossSize)
	crossCircle:setPosition(surface_width/2, surface_height/2)
	crossCircle:setColor(1,1,1,0)
	crossCircle.strokeEnabled = true
	crossCircle:setStrokeColor(0.4,0.4,0.4,0)
	crossCircle:setStrokeWidth(3)
	self.s:addEntity(crossCircle)
	self.crosshair = crossCircle
	
	local failSize = surface_height/10
	local failCircle = ScreenShape(ScreenShape.SHAPE_CIRCLE, failSize, failSize)
	local failLineSize = failSize/2/math.sqrt(2)
	local failSwipe = ScreenLine(Vector2(surface_width/2-failLineSize, surface_height/2+failLineSize), Vector2(surface_width/2+failLineSize, surface_height/2-failLineSize))
	self.fail = {failCircle, failSwipe}
	failCircle:setPosition(surface_width/2, surface_height/2)
	failCircle:setColor(1,1,1,0)
	failCircle.strokeEnabled = true
	failCircle:setStrokeColor(0.4,0.4,0.4,0)
	failCircle:setStrokeWidth(20)
	failSwipe:setColor(0.4,0.4,0.4,0)
	failSwipe:setLineWidth(20)
	for i,v in ipairs(self.fail) do
		self.s:addEntity(v)
	end
	
	local missSize = surface_height/40
	local missCircle = ScreenShape(ScreenShape.SHAPE_CIRCLE, missSize, missSize)
	missCircle:setPosition(surface_width/2, surface_height/2)
	missCircle:setColor(1,1,1,0)
	missCircle.strokeEnabled = true
	missCircle:setStrokeColor(0.4,0.4,0.4,1)
	missCircle:setStrokeWidth(5)
	missCircle.visible = false
	self.s:addEntity(missCircle)
	self.missHair = missCircle
end
function PulseEnt:onTick()
	if self.muted then return end

	local ticks = self:ticks() - self.lastMove
	self.lastIntensity = math.floor(ticks/120)%2==1 and self:intensity(ticks) or 0
	local intensity = self.lastIntensity
	if intensity == 0 and self.draining > 0 then 
		intensity = self.draining
		self.draining = self.draining - 0.2
	end
	if self.crashDrain > 0 then intensity = 0 end
	self.crosshair:setStrokeColor(0.4,0.4,0.4,intensity)
	
	local crashIntensity = 0
	if self.crashDrain >= 0 then
		crashIntensity = self.crashDrain
		if self:ticks() - self.crashed > 5 then
			self.crashDrain = self.crashDrain - 0.2
		end
	end
	self.fail[1]:setStrokeColor(0.4,0.4,0.4,crashIntensity)
	self.fail[2]:setColor(0.4,0.4,0.4,crashIntensity)
	
	local missSize = self.missed and (self:ticks()-self.missed)/4 or 2
	if missSize < 1 then
		local adjSize = surface_height/20 * missSize
		self.missHair:setShapeSize(adjSize,adjSize)
		self.missHair.visible = true
	else
		self.missHair.visible = false
		self.missed = nil
	end
	
	if self.cameraFrom then
		self.cameraProgress = self.cameraProgress + 1
		if self.cameraProgress >= km.sweepFrames then
			self.cameraFrom = nil self.cameraSpan = nil gm.forceCamera = nil gm.animateFreezeInput = false
		else
			local at = vAdd(self.cameraFrom, vMult(self.cameraSpan, self.cameraProgress/km.sweepFrames))
			gm.forceCamera = at
		end
		
		resetCamera()
	end
end
function PulseEnt:intensity(at,factor)
	at = at or self:ticks()
	factor = factor or 1
	return (-math.cos(at*math.pi / 60 * factor)+1)/2
end
function PulseEnt:mute()
	self.muted = true
	self.s.enabled = false
end
function PulseEnt:jolt()
	self.lastMove = self:ticks()
	if self.draining <= 0 then
		self.draining = self.lastIntensity or 0
	end
end
function PulseEnt:crash()
	self.crashed = self:ticks()
	self.lastMove = self.crashed
	self.crashDrain = 1
end
function PulseEnt:miss()
	self.missed = self:ticks()
	self:jolt()
end
function PulseEnt:animate(from, to)
	self.cameraFrom = vDup(standOn(from))
	self.cameraSpan = vDup(vSub(standOn(to), self.cameraFrom))
	self.cameraProgress = 0
	gm.animateFreezeInput = true
	gm.lastMouseAt = nil
end

class "BoxPulseEnt" (PulseEnt)
function BoxPulseEnt:BoxPulseEnt(spec)
	PulseEnt.PulseEnt(self,spec)
end
function BoxPulseEnt:onTick()
	if self.muted then return end
	
	if not (tm.shaderPulseConfig and tm.shaderPulseConfig.nobox) then
		if not self.hue then
			self.color = self.target[1]:getCombinedColor()
			self.hue = self.color:getHue() 
			self.saturation = self.color:getSaturation()
		end
		local frameColor = Color(0,0,0,255)
		frameColor:setColorHSV(self.hue, self.saturation, self:intensity() )
		for i,v in ipairs(self.target) do
			bridge:setColorObj(v, frameColor)
		end
	end
	
	PulseEnt.onTick(self)
end
function BoxPulseEnt:mute()
	if self.color then for i,v in ipairs(self.target) do
		bridge:setColorObj(v, self.color)
	end end
	PulseEnt.mute(self)
end

class "ShaderPulseEnt" (BoxPulseEnt)
function ShaderPulseEnt:ShaderPulseEnt(spec)
	BoxPulseEnt.BoxPulseEnt(self,spec)
	self:setTargetNumber()
end
function ShaderPulseEnt:setTargetNumber()
	local options = tm.shaderPulseConfig.targets
	self.targetNumber = options[math.random(#options)]
end
function ShaderPulseEnt:onTick()
	if self.muted then return end
	
	local ticks = self:ticks()
	local factor = tm.shaderPulseConfig.factor or 1
	if ticks%(60/factor) == 0 then self:setTargetNumber() end
	local intensity = self:intensity(ticks, 2*factor)
	local origValue = tm.gfx[tm.shaderPulseConfig.change]
	local value = origValue - intensity * (origValue - self.targetNumber)
	tm.shaderPulseConfig.changeFunc(tm.shaderPulseConfig.change, value)
	
	BoxPulseEnt.onTick(self)
end
function ShaderPulseEnt:mute()
	tm.shaderPulseConfig.changeFunc(tm.shaderPulseConfig.change, tm.shaderPulseConfig.final or tm.gfx[tm.shaderPulseConfig.change])
	BoxPulseEnt.mute(self)
end

tm.pulseClass = PulseEnt

function tm.pulser(spec)
	return tm.pulseClass(spec):insert()
end

-- Utility

function pointCollide(point)
	vSetPosition(gm.collider, point)
	return bridge:collidesWith(gm.s, gm.collider)
end
function standOn(hit)
	return vAdd(hit:getPosition(),Vector3(0,km.boxsize,0))
end
function boxCollide(hit)
	return pointCollide(standOn(hit))
end

-- Ordering

function findIdeal(t, func) -- General "pick best fit object from array". Function(a,b) returns true if a is better than b.
	local result = nil
	for i,v in ipairs(t) do
		if not (result and func(result, v)) then
			result = v
		end
	end
	return result
end

function distanceSort(one, two) -- Sort: Distance from center pole
	local function distance(box)
		local v = vSub(box:getPosition(), km.vzero)
		v.y = 0
		return vNorm(v)
	end
	return distance(one) < distance(two)
end

function pureDistanceSort(one, two) -- Sort: Distance from center
	local function distance(box)
		local v = vSub(box:getPosition(), km.vzero)
		return vNorm(v)
	end
	return distance(one) < distance(two)
end

function randomSort(one,two) -- Sort: Don't care
	return false
end

function makeAxisSort(axis) -- Sort generator: Magnitude on specified axis
	local function result(one,two)
		return one:getPosition()[axis] < two:getPosition()[axis]
	end
	return result
end

function makeSortNegate(f) -- Sort generator: Negate sort function
	local function result(one,two)
		return f(two,one)
	end
	return result
end

function collidesStart(e) -- Gate-- collides with already-set start?
	if boxCollide(e) then return true end
	if bridge:bindingId(gm.start) == bridge:bindingId(e) then return true end
	return false
end

function makeVisibleFromStart() -- Gate generator w/local memo
	local memo = {}
	local function visibleFromStart(e) -- Gate-- line of sight from already-set start?
		if collidesStart(e) then return true end

		local result = false
		local ebinding = bridge:bindingId(e)
		if memo[ebinding] == nil then
			local lookAt = standOn(e)
			vSetPosition(gm.collider, lookAt)
			gm.s:Update() -- Tried updating just collider but it didn't work?
			
			local hit = bridge:getFirstEntityInRay(gm.s, standOn(gm.start), lookAt) -- Use the collision object / standon not the cube!
			result = bridge:bindingId(hit) == bridge:bindingId(gm.collider) 
			memo[ebinding] = result
			
			-- if result then e:setColor(0, 0, 255,255) return true else e:setColor(0,255,0,255) end -- Debug
		else
			result = memo[ebinding]
		end
		
		return result -- Something is in the way-- acceptable
	end
	
	return visibleFromStart
end

function makeGatedSort(gate, f) -- Prefer entries for which the gate function does not return true. Tiebreak with f.
	local function result(one, two)
		local oneGated = gate(one)
		local twoGated = gate(two)
		if oneGated and not twoGated then return false end -- Only one is gated-- two is better
		if twoGated and not oneGated then return true end -- Only two is gated-- one is better
		return f(one, two)
	end
	return result
end

tm.startSort = makeGatedSort(boxCollide, makeSortNegate( distanceSort ) )
tm.flagpoleSort = makeGatedSort(collidesStart, makeSortNegate( makeAxisSort("z") ) )

-- PER-LEVEL OVERRIDES...

if fm.level == 2 then
	-- Level: Tower
	tm.startSort = makeGatedSort(boxCollide, makeAxisSort("y") )
	tm.flagpoleSort = makeGatedSort(collidesStart, makeSortNegate(makeAxisSort("y")) )
	function tm.boxposition(box, i)
		box:setPosition(randAround(0, 6, 7), randAround(0, tm.howtall()), randAround(0,6,7))
	end
	
	-- Appearance: Cluster
	tm.gfx = {blur=0.64, thresh=0.67, brighten=1.76}
	tm.au = {param={67.60, 1, 3}, iparam={4, 5256}, volume=0.5}
	tm.pulseClass = BoxPulseEnt
end

if fm.level == 3 then
	-- Level: Quadrants
	tm.startSort = makeGatedSort(boxCollide, makeAxisSort("x") )
	tm.flagpoleSort = makeGatedSort(makeVisibleFromStart(), randomSort)
	function tm.boxposition(box, i)
		box:setPosition(randAround((i%2)*8-4, 6, 7), randAround(0, tm.howtall()), randAround((math.floor(i/6)%2)*8-4, 6, 7))
	end
	
	-- Appearance: Whitespace
	tm.gfx = {blur=0, thresh=0.65, brighten=2.63}
	tm.au = {param={317.35, 0.2, 1}, iparam={3, 5256}, volume=0.5}
	function tm.boxcolor(box, i)
		local m = 1.0
		box:setMaterialByName("AtomMaterial")
		box:setColor(m, m, m, 1)
	end
	tm.pulseClass = ShaderPulseEnt
	tm.shaderPulseConfig = {changeFunc=rawendshader, change="thresh", targets = {0.70, 0.78, 0.70, 0.78, 0.39}, factor=0.25, forcefresh=true, nobox=true}
end

if fm.level == 4 then
	tm.startSort = makeGatedSort(boxCollide, pureDistanceSort)
	tm.flagpoleSort = makeGatedSort(collidesStart, randomSort)
	function tm.howmany()
		return 300
	end
	function tm.boxposition(box, i)
		box:setPosition(randAround(0, 12), randAround(0, 12), randAround(0, 12))
	end
	
	-- Appearance: Cluster
	tm.gfx = {blur=0.82, thresh=0.36, brighten=1.35}
	tm.au = {param={75.92, 4, 1}, iparam={3, 8192}, volume=0.1}
	tm.pulseClass = ShaderPulseEnt
	tm.shaderPulseConfig = {changeFunc=newblur, change="blur", targets = {0.57, 0.41, 0.06, tm.gfx.blur-0.005, tm.gfx.blur-0.01, tm.gfx.blur-0.02, tm.gfx.blur-0.03, tm.gfx.blur-0.05}, final=0.57}
end

-- ATTRACT

if fm.level == 0 then
	-- Level: Tower?
	tm.startSort = makeGatedSort(boxCollide, pureDistanceSort)
	tm.flagpoleSort = makeSortNegate(makeAxisSort("y"))
	function tm.boxposition(box, i)
		box:setPosition(randAround(0, 6, 7), randAround(0, tm.howtall()), randAround(0,6,7))
	end
	-- 46.5 -- 36 is better
	-- Appearance: Gray
	tm.gfx = {blur=0.93, thresh=0.23, brighten=1.64}
	tm.au = {param={30, 1, 1}, iparam={4, 8192}, volume=0.5}
	tm.pulseClass = nil
	function tm.boxcolor(box, i)
		local m = 0.5
		box:setMaterialByName("AtomMaterial")
		box:setColor(m, m, m, 1)
	end
	tm.demoMode = true
	tm.demoEnt = Ent({
		cam = function(self)
			gm.cam.x = ticks/4
			resetCamera()
		end,
		insert = function(self)
			killDos() dos = type_automaton() dos:insert()
			local gray = "FOUR#SHADES#OF#GRAY"
			dos:set(40-#gray,0,"FOUR#SHADES#OF#GRAY")
--			dos:set(0,3,"CONTROLS: MOUSE")
			dos:set(0,23,"RUNHELLO.COM")
			gm.cam.y = 36
			self:cam()
			return Ent.insert(self)
		end,
		onTick = function(self)
			self:cam()
		end,
		onInput = function(self)
			if pressed[0] then
				fm.level = fm.level + 1
				bridge:rebirth()
			end
		end
	})
end
