-- UI Test -- audio -- on load

if km.adhocui then

local au = {halfstep=math.pow(2,1/12)}

function pitchFrom(semitone)
	return math.pow(au.halfstep, semitone)
end

local sound = gm.snd

local window = r(UIWindow("Run", 200, 320))
uiscreen:addChild(window)
window:setPosition(surface_width-200-80,80)

setupWindow(window)

local param = function(t, _value)
    sound:setParam(t, _value)
end

local iparam = function(t, _value)
    sound:setIparam(t, _value)
end

local volparam = function(t, _value)
    sound:setVolume(_value)
end

local fny = {}
function funnyFreq(isAdj, value)
	fny[isAdj] = value
	if fny.base and fny.adj then
		param(0, fny.base*pitchFrom(fny.adj))
	end
end

local uicontents = {
	{"Base freq", target=0, act=param, kind="sliderex", min=1, max=1000, default=tm.au.param[1]},
--	{"Freq adj", target="adj", act=funnyFreq, kind="sliderex", min=-12, max=12, default=0, instant=true, round=true},
	{"Preamp", target=1, act=param, kind="sliderex", min=1, max=100000, default=tm.au.param[2]},
	{"Threshold", target=2, act=param, kind="sliderex", min=0, max=1, default=tm.au.param[3]},
	{"Note count", target=0, act=iparam, kind="sliderex", min=1, max=1000, default=tm.au.iparam[1], round=true},
	{"Note rpeg", target=1, act=iparam, kind="sliderex", min=1, max=1024*8, default=tm.au.iparam[2], round=true},
--	{"Reset freq", target="noisesteps", act=iparam, kind="sliderex", min=0, max=1000, default=gm.machine.noisesteps, round=true},
--	{"Noise often", target="noiserepeats", act=iparam, kind="sliderex", min=1, max=1000, default=gm.machine.noiserepeats, round=true},
--	{"Resolution", target="resolution", act=iparam, kind="sliderex", min=1, max=16, default=gm.machine.resolution, round=true},
	{"Volume", target=5, act=volparam, kind="sliderex", min=0, max=1, default=tm.au.volume},
}

local layout = Layout({target=window, contents=uicontents}):insert()

--window:hideWindow()

end