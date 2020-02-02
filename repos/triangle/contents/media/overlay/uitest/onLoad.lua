-- Audio UI -- on load

if adhocui then

local window = r(UIWindow("Machine", 200, 320))
uiscreen:addChild(window)
window:setPosition(surface_width-200-40,40)

registerPopupEnt(window, KEY_F1)

local iparam = function(t, _value)
    gm.controller:iparam(t, _value)
end

local volparam = function(t, _value)
    gm.controller.sound:setVolume(_value)
end

local uicontents = {
	{"Loop length", target=gm.controller, act=gm.controller.setLoopsize, kind="sliderex", min=1, max=2048, default=gm.machine.loopsize, round=true},
	{"Audio Symbols", target=gm.controller, act=gm.controller.setSymbolcount, kind="sliderex", min=2, max=256, default=gm.machine.symbolcount, round=true},
	{"Machine States", target=gm.controller, act=gm.controller.setStatecount, kind="sliderex", min=2, max=256, default=gm.machine.statecount, round=true},
}

local layout = Layout({target=window, contents=uicontents}):insert()

--window:hideWindow()

end