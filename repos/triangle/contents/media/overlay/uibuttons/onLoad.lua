-- Audio Buttons -- on load

if adhocui then

local window = r(UIWindow("Controls", 100, 160))
uiscreen:addChild(window)
window:setPosition((surface_width-100-40),(surface_height-160-40))

registerPopupEnt(window, KEY_F2)

local replay = function()
    gm.controller:recreate()
    bridge:unfocus()
end

local quiet = function()
    if gm.controller.sound:isPlaying() then
        gm.controller.sound:Stop()
    else
        gm.controller.sound:Play()
    end
    bridge:unfocus()
end

local stuck = function()
    gm.controller:unstick()
    bridge:unfocus()
end

local shuffle = function()
    gm.controller.machine:statescramble()
    gm.controller:uploadSymbols()
    bridge:unfocus()
end

local scramble = function()
    gm.controller:loopScramble()
    bridge:unfocus()
end

local randomize = function()
    gm.controller.machine:randomize()
    gm.controller:recreate()
    bridge:unfocus()
end

local uicontents = {
    {"Replay", act=replay, kind="trigger"},
    {"Pause", act=quiet, kind="trigger"},
    {"Stuck?", act=stuck, kind="trigger"},
    {"Scramble loop", act=scramble, kind="trigger"},
    {"Shuffle states", act=shuffle, kind="trigger"},
    {"New random", act=randomize, kind="trigger"},
}

local layout = Layout({target=window, contents=uicontents}):insert()

--window:hideWindow()

end