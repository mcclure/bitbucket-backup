-- On Load

cm = {tx = 90, ty = 0}
gm = gm or {}
pull(km, {whenFlip = 30/2, pauseFor=0, humsweep = 0.005})

local s = r(Scene())
gm.s = s

s:getDefaultCamera():setPostFilter("Blur")
shaders = {}
shaders[1] = shaderBindings(s:getDefaultCamera(),
					{blurSize=1/256.0}, 0)
shaders[2] = shaderBindings(s:getDefaultCamera(),
					{blurSize=1/256.0, brighten=1, thresh=1}, 1)

--s:getDefaultCamera():setPostFilter("FilterMaterial")
--shader = shaderBindings(s:getDefaultCamera(),
--                       {radius=3/surface_width/2, aspect=(surface_width/surface_height)})


local e = SceneEntity()
s:addEntity(e)

local res = 4
local base = Blocker()
local b = {}
base.vox[1][1][1] = true

local accum = {-.5}
for i = 1,res do
	local d = math.pow(0.5,i)
	table.insert(accum, accum[#accum] + d)
end

local camera = s:getDefaultCamera()
camera:setPosition(2,0,0)
camera:lookAt(a(Vector3(0,0,0)),a(Vector3(0,1,0)))

function updateCamera()
	e:setYaw(cm.tx)
	e:setPitch(cm.ty)
end
updateCamera()

Ent({pausing = 0, focus = 1, humticks = 1, humvol = 0,
	reset = function(self)
		self.focus = 1
		self.pausing = km.pauseFor
	end,
	onTick = function (self)
		local humgoal = (math.floor((self.humticks-1)/4) == 1) and 1 or 0
		if self.humvol < humgoal then self.humvol = self.humvol + km.humsweep
		elseif self.humvol > humgoal then self.humvol = self.humvol - km.humsweep end
		
		if (ticks%km.whenFlip == 0) then
			if self.pausing > 0 then
				self.pausing = self.pausing - 1
				if self.pausing == 0 then 
					self.humticks = self.humticks + 1
					if self.humticks > 8 then self.humticks = 1 end
				end
			else			
				if b[self.focus] then b[self.focus]:remove() end
				b[self.focus] = (b[self.focus-1] or base):subdivide()
				
				local d = math.pow(0.5,self.focus)
				b[self.focus]:add({e=e,d=d,o=-0.5,zo=accum[self.focus],g=(self.focus/res)})
				
				self.focus = self.focus + 1
				if self.focus > res then self:reset() end
			end
		end
		
		if gm.spin then
			cm.tx = cm.tx + 1/8
			updateCamera()
		end
	end,
	onInput = function (self)
		if down[0] then
			local off = mouseDownAt.x - mouseAt.x
			cdelete(mouseDownAt) mouseDownAt = vDup(mouseAt)
			cm.tx = cm.tx - off/4
			updateCamera()
		end
end
}):insert()

-- Set up input handler

down = {} pressed = {}
listeners = {} -- To get unicode-level key input
mouseDownAt = nil
mouseAt = nil

class "Input" (EventHandler)
function Input:Input()
	EventHandler.EventHandler(self)
end

function Input:handleEvent(e)
	if self.disabled then return end
	local inputEvent = InputEvent(e)
	local key = inputEvent:keyCode()
	if e:getEventCode() == InputEvent.EVENT_MOUSEMOVE then
		cdelete(mouseAt) mouseAt = inputEvent:getMousePosition()
	elseif e:getEventCode() == InputEvent.EVENT_MOUSEDOWN then
		cdelete(mouseDownAt) mouseDownAt = inputEvent:getMousePosition()
		cdelete(mouseAt) mouseAt = vDup(mouseDownAt)
		down[inputEvent.mouseButton] = true
		pressed[inputEvent.mouseButton] = true
	elseif e:getEventCode() == InputEvent.EVENT_MOUSEUP then
		down[inputEvent.mouseButton] = nil
	elseif e:getEventCode() == InputEvent.EVENT_KEYDOWN then
		down[key] = true
		pressed[key] = true
		for k,v in pairs(listeners) do
			v:push({key, bridge:charCode(inputEvent)})
		end
	elseif e:getEventCode() == InputEvent.EVENT_KEYUP then
		down[key] = false
	end
end

input = Input()

if false then
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEMOVE)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEDOWN)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEUP)
end
Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYDOWN)
Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYUP)

-- "UI"

if 1 then

--input.disabled = true

uiscreen = r(Screen())

uinet = r(ScreenEntity())
uinet.processInputEvents = true
uinet.visible = false
--uinet:setPositionMode(ScreenEntity.POSITION_TOPLEFT)
uinet:setPosition(surface_width/2,surface_height/2)
uiscreen:addChild(uinet)
uinet:setHitbox(surface_width,surface_height)
uinet:addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEMOVE)
uinet:addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEDOWN)
uinet:addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEUP)

window = r(UIWindow("CONTROLS", 200, 255))
uiscreen:addChild(window)
window:setPosition(50,surface_height-255-50)

if true then
local ke = Ent({listener = Queue(), onInput = function (self)
	local is = false
	while true do
		local entry = self.listener:pop()
		if not entry then break end
		if entry[1] ~= KEY_F8 then is = true end -- Ignore screenshots
	end
	if is and self.windowHidden then window:showWindow() self.windowHidden = false end
end})
window:addEventListener(nil, function()
	window:hideWindow()
	ke.windowHidden = true
end, UIEvent.CLOSE_EVENT)
table.insert(listeners, ke.listener)
ke:insert()
end

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
	
	local portion = 0.7
	local voloff = 1-clamp(0,blurm.blur/portion-(1-portion),1)
	for i,v in ipairs(au.tones)   do v:setVolume(voloff)   end
	for i,v in ipairs(au.squares) do v:setVolume(1-voloff) end
end

function rawendshader(t, value)
	shaders[2][t]:set(value)
end

local uicontents = {
	{"Blur", target="blur", act=newblur, kind="sliderex", valuekind="Number", min=0, max=1, default=0.807, instant=true},
	{"Stretch", target="stretch", act=newblur, kind="sliderex", min= -1, max=1, default=0, instant=true},
	{"Threshold", target="thresh", act=rawendshader, kind="sliderex", min= 0, max=1, default=0.20, instant=true},
	{"Brighten", target="brighten", act=rawendshader, kind="sliderex", min=1, max=8, default=1.35, instant=true},
	{"Auto-rotate", act=function(_, v) gm.spin = v end, kind="check", ystill=true},
	{"Reset", act=function() gm.pleasereset = true end, kind="trigger", ystill=true},
}

layout = Layout({target=window, contents=uicontents}):insert()

end