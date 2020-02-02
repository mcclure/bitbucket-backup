addScreen("rroad", km.roomObj)

if not au.rainbow:isPlaying() then
	au.rainbow:Play(true)
end
au.rainbow:setVolume(1.0)

gm.rays = {id("ray1"),id("ray2"),id("ray3"),id("ray4"),id("ray5")}
gm.ray22 = id("ray2-2")
if not fm.raycolor then
	fm.raycolor = {}
	for i,v in ipairs(gm.rays) do
		table.insert(fm.raycolor, v:getCombinedColor())
	end
end
if not fm.raytime then
	pull(fm, {raytime=MiniTimer(km.rayon), raytick=0,rayrot=0,})
end

function rayColor()
	if fm.raytime:get() then
		if fm.raytick%6 > 1 then
			fm.rayrot = fm.rayrot + 1
			for i,v in ipairs(gm.rays) do
				local wantcolor = fm.raycolor[ (i + fm.rayrot) % 5 + 1 ]
				bridge:setColorObj(v, wantcolor)
				if i == 2 then
					bridge:setColorObj(gm.ray22, wantcolor)
				end
			end
		end
		fm.raytick = fm.raytick + 1
	end
end

rayColor()