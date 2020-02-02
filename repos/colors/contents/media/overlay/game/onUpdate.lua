-- On Update

dos.g:pxmix_ryb(gm.ls.r.g, gm.ls.g.g, gm.ls.b.g)
--dos.g:pxcopy_rgb(gm.ls.r.g, 1)
--dos.g:pxcopy_rgb(gm.ls.g.g, 2)
--dos.g:pxcopy_rgb(gm.ls.b.g, 4)
for k,v in pairs(gm.ls) do
	v:reblank()
end

pressed = {} -- Must be in onUpdate if "pressed" is being used