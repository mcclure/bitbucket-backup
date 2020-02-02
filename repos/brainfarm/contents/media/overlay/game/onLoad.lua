-- On Load

-- A sparse 2D grid. Should go in project_lua?
class "SparseGrid"

function SparseGrid:SparseGrid()
	self.data = {}
end

function SparseGrid:set(x, y, value)
	if not self.data[y] then self.data[y] = {} end
	local current = self.data[y][x]
	if current then
		s:removeChild(current)
		delete(current)
	end
	self.data[y][x] = value
end
function SparseGrid:get(x, y, default)
	local yv = self.data[y]
	local xv = yv and yv[x]
	if not xv and default then self:set(x,y,default) return default end
	return xv
end
function SparseGrid:clear_row(y)
	local yv = self.data[y]
	if yv then
		for x,xv in pairs(yv) do -- Assume grid is full of ScreenEchos
			s:removeChild(xv)
			delete(xv)
		end
		self.data[y] = nil
	end
	resetPtrLookup()
end

-- Setup

-- Konstant machine, camera machine, game machine
km = {scale = 4, startblocks = 16, startblockrange = 0.5, blockspeed = 2, scrollspeed = 1, swervedanger = 4, soundfade = 8*60}
km.block_unit = 8*km.scale
km.block_height = math.floor(surface_height/km.block_unit + 0.5)
km.block_width = km.block_height/2
km.blockrollover = math.floor(km.block_unit/km.blockspeed)
km.scrollrollover = math.floor(km.block_unit/km.scrollspeed)
km.soundpause = km.soundfade/2
cm = {}
gm = {iscroll = 0, bscroll = 0}

-- Template images
Services.Renderer:setTextureFilteringMode(Renderer.TEX_FILTERING_NEAREST)
headt = r(ScreenImage("media/head.png"))
img = {}
for i,v in ipairs({"rr","uu","ul","lu","ur","ru"}) do
	img[v] = r(ScreenImage(string.format("media/%s.png", v)))
end
img["ll"] = img["rr"]

-- Display stuff
gm.heads = {}
gm.tails = SparseGrid()

s = r(Screen())
se = r(Screen())

function screenOffsets(x,y)
	s:setScreenOffset(x,y) se:setScreenOffset(x,y)
end
cm.lasty = surface_height 
cm.lastx = (surface_width - km.block_width*km.block_unit)/2
function vScroll(y)
	gm.iscroll = gm.iscroll + y
	if gm.iscroll >= km.scrollrollover then	
		gm.tails:clear_row(gm.bscroll-1)
		gm.bscroll = gm.bscroll + 1
		gm.iscroll = 0
	end
	cm.lasty = cm.lasty + y*km.scrollspeed
	screenOffsets(cm.lastx,cm.lasty)
end
vScroll(0)

function blockPosition(e, bx, by, ex, ey) -- entity, block x, block y, extra x, extra y
	e:setPosition(bx*km.block_unit + (ex or 0), -(by+1)*km.block_unit + (ey and -ey or 0))
end
function letterfor(h)
	if h then
		if h > 0 then return "r"
		else return "l" end
	else return "u" end
end
function imgfor(lasth, h)
	local name = letterfor(lasth) .. letterfor(h)
	return img[ name ]
end
function stamp(head)
	local tail = ScreenEcho( imgfor(head.lasth, head.h) )
	tail:setScale(km.scale,km.scale) tail:setPositionMode(ScreenEntity.POSITION_TOPLEFT)
	blockPosition(tail, head.bx, head.by)
	s:addChild(tail)
	gm.tails:set(head.bx,head.by,tail)
end

do 
	local startmax = math.floor(km.block_height * km.startblockrange)
	local startheight = math.floor((km.block_height - startmax)/2)
	for i=1,km.startblocks do
		local head = ScreenEcho(headt) -- head.h implicitly null
		head:setScale(km.scale,km.scale)
		head.bx, head.by = math.random(km.block_width)-1,startheight+math.random(startmax)-1
		head.i = 0
		blockPosition(head, head.bx, head.by)
		stamp(head)
		se:addChild(head)
		table.insert(gm.heads, head)
	end
end

-- Generate "hum" sound

sm = {snd = {}, vols = {1,0,0}, wantvols = {0,1,0}}

function clamp(low, v, high)
	if low > v then return low end
	if high < v then return high end
	return v
end

for i, amp in ipairs({1,2,4}) do
	local data = a(NumberArray())
	local limit = 16384
	for i=0,limit do
		local v = math.sin(i / 44100.0 * 2*math.pi * 110 / sqrt(2)) * math.sin(i / 8192 * math.pi)
		v = v * amp
		
		local v2 = math.sin(i / 44100.0 * 2*math.pi * 55 / sqrt(2)) * math.sin(i / 16384 * math.pi)
		v2 = v2 * amp
		
		data:push_back(clamp(-1, v + v2, 1))
	end
	local humEffect = r(bridge:soundFromValues(data))
	humEffect:setVolume(sm.vols[i])
	table.insert(sm.snd, humEffect)
end
for i,v in ipairs(sm.snd) do
	v:Play(true)
end
function soundFix()
	local soundwrap = km.soundfade + km.soundpause
	local soundAt = ticks % soundwrap
	local progress = (soundAt - km.soundpause) / km.soundfade
	if progress < 0 then progress = 0 end
	if progress > 1 then progress = 1 end
	if ticks > 0 and soundAt == 0 then
		sm.vols = sm.wantvols
		sm.wantvols = {0,0,0}
		while true do -- repeat until anyflip + any
			local anyflip = false
			local any = false
			for i=1,3 do
				local was = math.random(2) == 1
				if was then
					sm.wantvols[i] = 1-sm.vols[i]
					any = any or sm.wantvols[i] > 0.5
					anyflip = true
				end
			end
			if anyflip and any then break end
		end
	end
	for i,v in ipairs(sm.snd) do
		v:setVolume(sm.vols[i]*(1-progress) + sm.wantvols[i]*(progress))
	end
end
soundFix()

-- Set up input handler
down = {}
pressed = {}
mouseAt = Vector3(0,0,0)
class "Input" (EventHandler)
function Input:Input()
	EventHandler.EventHandler(self)
end

function Input:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		if e:getEventCode() == InputEvent.EVENT_MOUSEMOVE then
			delete(mouseAt)
			mouseAt = inputEvent:getMousePosition()
		elseif e:getEventCode() == InputEvent.EVENT_MOUSEDOWN then
			down[inputEvent.mouseButton] = true
		elseif e:getEventCode() == InputEvent.EVENT_MOUSEUP then
			down[inputEvent.mouseButton] = nil
		elseif e:getEventCode() == InputEvent.EVENT_KEYDOWN then
			local key = inputEvent:keyCode()
			pressed[key] = true
		end
	end
end

do
	input = Input()

	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_MOUSEMOVE)
	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_MOUSEDOWN)
	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_MOUSEUP)
	Services.Core:getInput():addEventListener(input, InputEvent.EVENT_KEYDOWN)
end