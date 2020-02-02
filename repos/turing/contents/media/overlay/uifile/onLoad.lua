-- Audio Buttons -- on load

if adhocui then

local window = r(UIWindow("Files", 100, 85))
uiscreen:addChild(window)
window:setPosition((surface_width-100-175),(surface_height-85-40))

registerPopupEnt(window, KEY_F2)

function endsWith(String,End) -- via lua-users.org
   return End=='' or string.sub(String,-string.len(End))==End
end
function forceEnding(String,End)
	if not endsWith(String,End) then String = String .. End end
	return String
end

local save = function()
    local filename = Services.Core:saveFilePicker()
    print(filename)
    if filename and #filename>0 then
		filename = forceEnding(filename, ".tau")
		print(filename)
        bridge:saveTableIntoFile(filename, "tau", gm.machine)
        bridge:unfocus()
    end
end

local open = function()
    local filename = bridge:oneFilePicker("tau")
    print(filename)
    if filename and #filename>0 then
        local spec = bridge:loadTableFromFile(filename);
        nextmachine = spec
		wantrebirth = true
    end
end

local uicontents = {
    {"Save", act=save, kind="trigger"},
    {"Open", act=open, kind="trigger"},
}

local layout = Layout({target=window, contents=uicontents}):insert()

--window:hideWindow()

end