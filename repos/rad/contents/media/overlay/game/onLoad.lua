-- On Load

-- Globals
s = scene()
down = {}
mouseDown = {}
moveby = 0.1
inset = 1.0-moveby/4
did_win = false

autorelease = {} -- What if already exists?
retain = {}

onground = false
downaccel = -0.02
downcap = -3*moveby
downv = -moveby
uponjump = 2*moveby - downaccel

-- Some utility functions

function a(v)
	table.insert(autorelease, v)
	return v
end
function r(v)
	table.insert(retain, v)
	return v
end
function i(v)
	return v
end
function vDup(v)
	return Vector3(v.x,v.y,v.z)
end
function vAdd(v,b)
	return a(Vector3(v.x+b.x,v.y+b.y,v.z+b.z))
end
function vSub(v,b)
	return a(Vector3(v.x-b.x,v.y-b.y,v.z-b.z))
end
function vSetPosition(e,v)
	e:setPosition(v.x,v.y,v.z)
end

-- Construct "echo block"
echoBlock = bridge:echoBlock()
visibleBlock = r(ScenePrimitive(TYPE_BOX, 1.0,1.0,1.0)) -- Everything
visibleBlock:setMaterialByName("GroundMaterial")
echoBlock:addEntity(visibleBlock)
function mutateVisibleBlock()
	local x = math.random()
	visibleBlock:setColor(x,x,x,1)
end
mutateVisibleBlock()

-- Construct "moving blocks"
moving = {}
do
	local blocks = bridge:carpetsAt()
	local colors = bridge:diamondsAt()
	for i = 0,blocks:size()-1 do
		local block = blocks:get(i)
		local color = colors:get(i)
		block.by = color.x
		block.as = color.y
		table.insert(moving, block)
	end
end

-- Make some rotation matrices.
camerastep = 8
cameradist = r(Vector3(7,7,7))

-- Make things
p = r(ScenePrimitive(TYPE_BOX, 1.0,1.0,1.0)) -- Player
pat = bridge:playerAt()
pat.y = pat.y-- * 5
vSetPosition(p, pat)
p:setColor(1,1,1,1)
p:setMaterialByName("CubeMaterial")
s:addCollisionChild(p, SHAPE_BOX, 1.0)

exits = {}
do
	local diamonds = bridge:diamondsAt()
	for i = 0,diamonds:size()-1 do
		local exit = diamonds:get(i)
		local p = r(ScenePrimitive(TYPE_BOX, 1.0,1.0,1.0)) -- Exit
		vSetPosition(p,exit)
		p:setColor(0,0,0,1)
		p:setMaterialByName("CubeMaterial")
		s:addCollisionChild(p, SHAPE_BOX, 1.0)
		table.insert(exits, p)
	end
end

-- Camera

s:getDefaultCamera():setPosition(pat.x+7,7,pat.z-3+7)
s:getDefaultCamera():lookAt(a(Vector3(pat.x,0,pat.z-3)),a(Vector3(0,1,0)))

-- Music?
snd_jump = r(Sound("media/doorslam.ogg"))
snd_land = r(Sound("media/jdrums23.ogg"))
howmanyloops = 4
loopcount = 0
nextloopat = 0

function new_loop()
	local oneloop = r(Sound("media/simple_loop.ogg"))
	oneloop:setPitch(0.1 + 0.1/20*loopcount)
	oneloop:Play(true)
	nextloopat = nextloopat + 115/howmanyloops
	loopcount = loopcount + 1
end
new_loop()

-- Keyboard handler
class "Keyer" (EventHandler)
function Keyer:Keyer()
	self:EventHandler()
end

function Keyer:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == EVENT_KEYDOWN then
			down[key] = true
		elseif e:getEventCode() == EVENT_KEYUP then
			down[key] = false
		end
	end
end

do
	local keyer = Keyer()

	Services.Core:getInput():addEventListener(keyer, EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(keyer, EVENT_KEYUP)
end