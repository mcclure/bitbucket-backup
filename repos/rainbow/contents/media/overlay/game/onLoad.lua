-- On Load

pull(gm, {is=true})
Services.Renderer:setClearColor(0.2,0.2,0.2,0)

-- Set up "spotlight" effect
gm.s = r(PhysicsScreen(10,60))
gm.s:setGravity(Vector2(0,0))

Services.Renderer:setTextureFilteringMode(Renderer.TEX_FILTERING_LINEAR)

gm.s:setScreenShader("FilterMaterial")

shader = shaderBindings(gm.s,
	{v1=Vector2(0.1,0.1), v2=Vector2(0.1,0.1), v3=Vector2(0.1,0.1), v4=Vector2(0.1,0.1), radius=0.1, intensity=-0.1, aspect=(surface_width/surface_height)})

--addPhysicsChild(SceneEntity *newEntity, int type=0, Number mass = 0.0f, Number friction=1, Number restitution=0, int group=1, bool compoundChildren = false);		

function makeWall(width, height, atx, aty) 
	local whaal = ScreenShape(ScreenShape.SHAPE_RECT, width, height)
	whaal:setPosition(atx,aty)
	gm.s:addPhysicsChild(whaal, PhysicsScreenEntity.ENTITY_RECT, true)
end

local offer = 10
makeWall(surface_width+offer, offer*2, surface_width/2, -offer*2)
makeWall(surface_width+offer, offer*2, surface_width/2, surface_height+offer*2)
makeWall(offer*2, surface_height, -offer*2, surface_height/2)
makeWall(offer*2, surface_height+offer, surface_width+offer*2, surface_height/2) 

function makeAt(name, scale, x, y, v)
	Services.Renderer:setTextureFilteringMode(Renderer.TEX_FILTERING_NEAREST)
	local wheel = ScreenImage(name)
	wheel:setPosition(x,y)
	wheel:setScale(scale,scale)
	gm.s:addPhysicsChild(wheel, PhysicsScreenEntity.ENTITY_CIRCLE, false)

	local e = Ent({e=wheel,
		onTick = function(self) 
			-- Recenter shader focus
			local p = self.e:getPosition()
			local center = a(v:get())
			center.x = p.x / surface_width
			center.y = 1-p.y / surface_height
			v:set(center)
		end,
		onInput = function(self)
			if down[0] then
				local p = self.e:getPosition()
				local x = mouseAt.x - p.x
				local y = mouseAt.y - p.y
				gm.s:applyForce(self.e, x*15, y*15)
			end
		end
	}):insert()
	return e
end

gm.p = makeAt("media/colorfull.png", 1, 0.1*surface_width, 0.1*surface_height, shader.v1)

gm.all = {gm.p}
table.insert(gm.all, makeAt("media/colororange.png", 0.5, 0.6*surface_width, 0.3*surface_height, shader.v2))
table.insert(gm.all, makeAt("media/colorblue.png", 0.5, 0.8*surface_width, 0.8*surface_height, shader.v3))
table.insert(gm.all, makeAt("media/colorpurple.png", 0.5, 0.3*surface_width, 0.7*surface_height, shader.v4))

Services.Renderer:setTextureFilteringMode(Renderer.TEX_FILTERING_LINEAR)

local radmin, radmax, intmin, intmax = -3, 3, -1, 1
local soundname = {"andiblingy", "andibLop", "gongz", "prettylittlething"}
if not au then
	au = {}
	sound = {}
	for i,v in ipairs(soundname) do
		au[v] = Sound(string.format("media/audio/%s.ogg", v))
		au[v]:Play(true)
		table.insert(sound, au[v])
	end
end
Ent({onTick = function(self)
	local radv = (shader.radius:get()-radmin)/(radmax-radmin)
	local intv = (shader.intensity:get()-intmin)/(intmax-intmin)
	
	sound[1]:setVolume(gm.is and intv or 0)
	sound[2]:setVolume(gm.is and (1-intv) or 0)
	sound[3]:setVolume((1-radv) * 0.75)
	sound[4]:setVolume(radv)
	
--	for i,v in ipairs(sounds) do local snd = au[v] end
end}):insert()


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

local havegui = true -- _DEBUG

do
	input = Input()

	if not havegui then
		Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEMOVE)
		Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEDOWN)
		Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEUP)
	end
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYUP)
end

if havegui then 

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

window = r(UIWindow("CONTROLS", 200, 180))
uiscreen:addChild(window)
window:setPosition(surface_width-200-75,50)

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

function rawendshader(t, value)
	shader[t]:set(value)
end

function allvis(_, v) local is = v and 1 or 0 for i,e in ipairs(gm.all) do e.e:setColor(is,is,is,is) gm.is = v end end
function allrot(_, v) for i,e in ipairs(gm.all) do gm.s:setAngularVelocity(e.e, v and (math.random()-0.5)*3 or 0) end end

local uicontents = {
	{"radius", target="radius", act=rawendshader, kind="sliderex", min= -3, max=3, default=0.20, instant=true},
	{"intensity", target="intensity", act=rawendshader, kind="sliderex", min=-1, max=1, default=0.1, instant=true},
	{"Visible", act=allvis, kind="check", default=true},
	{"Auto-rotate", act=allrot, kind="check", ystill=true},
	{"Reset", act=function() gm.pleasereset = true end, kind="trigger", ystill=true},
}

layout = Layout({target=window, contents=uicontents}):insert()

end