-- Audio UI -- on load

if adhocui then

local window = r(UIWindow("Run", 200, 320))
uiscreen:addChild(window)
window:setPosition(surface_width-200-80,80)

registerPopupEnt(window, KEY_F1)

local iparam = function(t, _value)
    gm.controller:iparam(t, _value)
end

local volparam = function(t, _value)
    gm.controller.sound:setVolume(_value)
end

local uicontents = {
	{"Machine steps", target="steps", act=iparam, kind="sliderex", min=0, max=1000, default=gm.machine.steps, round=true},
	{"Machine often", target="repeats", act=iparam, kind="sliderex", min=1, max=1000, default=gm.machine.repeats, round=true},
	{"Noise steps", target="noisesteps", act=iparam, kind="sliderex", min=0, max=1000, default=gm.machine.noisesteps, round=true},
	{"Noise often", target="noiserepeats", act=iparam, kind="sliderex", min=1, max=1000, default=gm.machine.noiserepeats, round=true},
	{"Resolution", target="resolution", act=iparam, kind="sliderex", min=1, max=16, default=gm.machine.resolution, round=true},
	{"Volume", target=5, act=volparam, kind="sliderex", min=0, max=1, default=1},
}

local layout = Layout({target=window, contents=uicontents}):insert()

--window:hideWindow()

end