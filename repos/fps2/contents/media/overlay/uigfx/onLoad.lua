-- UI Test -- graphics -- on load

if km.adhocui then

--input.disabled = true

local window = r(UIWindow("CONTROLS", 200, 255))
uiscreen:addChild(window)
window:setPosition(50,surface_height-255-50)

setupWindow(window)

function rawendshader(t, value)
	shaders[2][t]:set(value)
end

local uicontents = {
	{"Blur", target="blur", act=newblur, kind="sliderex", valuekind="Number", min=0, max=1, default=tm.gfx.blur},
--	{"Stretch", target="stretch", act=newblur, kind="sliderex", min= -1, max=1, default=0, instant=true},
	{"Threshold", target="thresh", act=rawendshader, kind="sliderex", min= 0, max=1, default=tm.gfx.thresh},
	{"Brighten", target="brighten", act=rawendshader, kind="sliderex", min=1, max=8, default=tm.gfx.brighten},
--	{"Auto-rotate", act=function(_, v) gm.spin = v end, kind="check", ystill=true},
--	{"Reset", act=function() gm.pleasereset = true end, kind="trigger", ystill=true},
}

layout = Layout({target=window, contents=uicontents}):insert()

end