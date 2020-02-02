-- Audio UI -- on load

if true and _DEBUG then 

uiscreen = r(Screen())

if false then -- No mouse needed
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
end

window = r(UIWindow("CONTROLS", 200, 180))
uiscreen:addChild(window)
window:setPosition(surface_width-200-75,50)

if true then
local ke = Ent({listener = Queue(), windowHidden = true, onInput = function (self)
	local is = false
	while true do
		local entry = self.listener:pop()
		if not entry then break end
		if entry[1] == KEY_p then is = true end -- p key to bring up
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

function rainparam(t, value)
	gm.au.rain:setParam(t, value)
end

function allvis(_, v) local is = v and 1 or 0 for i,e in ipairs(gm.all) do e.e:setColor(is,is,is,is) gm.is = v end end
function allrot(_, v) for i,e in ipairs(gm.all) do gm.s:setSpin(e.e, v and (math.random()-0.5)*3 or 0) end end

local uicontents = {
	{"pitch", target=gm.au.rain, act=gm.au.rain.setPitch, kind="sliderex", min= 0, max=30, default=gm.au.rain:getPitch(), instant=false},
	{"period", target=0, act=rainparam, kind="sliderex", min=0, max=150, default=gm.au.rain:getParam(0), instant=false},
	{"intensity", target=1, act=rainparam, kind="sliderex", min=1, max=10, default=gm.au.rain:getParam(1), instant=false},
}

layout = Layout({target=window, contents=uicontents}):insert()

window:hideWindow()

end