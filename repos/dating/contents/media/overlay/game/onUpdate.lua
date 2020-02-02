-- On Update

gm.ls:reblank()
gm.p:draw()
gm.ls:flushrain(gm.gfx.rain)

if gm.needexit then gm.needexit(bridge) end

pressed = {} -- Must be in onUpdate if "pressed" is being used