-- Audio Buttons -- on load

if adhocui then

local window = r(UIWindow("File", 100, 85))
uiscreen:addChild(window)
window:setPosition((surface_width-100-40),(surface_height-85-40))

registerPopupEnt(window, KEY_F2)

local uicontents = {
    {"Revert", target=gm.controller, act=gm.controller.file_revert, kind="trigger"},
    {"Save", target=gm.controller, act=gm.controller.file_save, kind="trigger"},
}

local layout = Layout({target=window, contents=uicontents}):insert()

--window:hideWindow()

end