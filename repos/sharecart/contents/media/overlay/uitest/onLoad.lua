-- Audio UI -- on load

if adhocui then

local window = r(UIWindow("Synth", 200, 320))
uiscreen:addChild(window)
window:setPosition(surface_width-200-40,40)

registerPopupEnt(window, KEY_F1)

local snaps = {0,1}
for i,v in ipairs({3,5,8}) do
	for v2=1,v-1 do
		table.insert(snaps, v2/v)
	end
end
table.sort(snaps)
--print(snaps)

local nofilter = function (x) return x end
local curvefilter = function (x) return math.pow(16, x) end
local cubefilter  = function (x) return 2*x*x*x end
local dist = function (x,y) return math.abs(x-y) end
local snapfilter = function (x)
	local best, bestdist = 0, 100000000
	for i,v in ipairs(snaps) do
		local thisdist = dist(v, x)
		if thisdist < bestdist then
			best = v bestdist = thisdist
		end
	end
	return best
end

local vfilters = {
	curvefilter,
	nofilter,
	cubefilter,
	snapfilter,
	snapfilter,
	snapfilter,
}

function rainparam(t, _value)
	local value = _value
	if vfilters[t+1] then value = vfilters[t+1](value) end
	for i,v in ipairs(gm.controller.sounds) do
		v:setParam(t, value)
	end
	gm.controller.song.param[t+1] = _value
end

local uicontents = {
	{"Overdrive", target=0, act=rainparam, kind="slider", min=0, max=1, default=gm.controller.song.param[1], instant=true},
	{"Tempo", target=1, act=rainparam, kind="slider", min=0, max=1, default=gm.controller.song.param[2], instant=true},
	{"Chaos", target=2, act=rainparam, kind="slider", min=0, max=1, default=gm.controller.song.param[3], instant=true},
	{"Harmonic1", target=3, act=rainparam, kind="slider", min=0, max=1, default=gm.controller.song.param[4], instant=true},
	{"Harmonic2", target=4, act=rainparam, kind="slider", min=0, max=1, default=gm.controller.song.param[5], instant=true},
	{"Harmonic3", target=5, act=rainparam, kind="slider", min=0, max=1, default=gm.controller.song.param[6], instant=true},
}

layout = Layout({target=window, contents=uicontents}):insert()

--window:hideWindow()

end