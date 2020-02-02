addScreen("airlock", km.roomObj)

if not au.alone then
	au.alone = Sound("media/ArtieShaw-AloneTogether1939.ogg")
end
if not aq and not au.alone:isPlaying() then -- FOR DEBUGGING-- REMOVE
	au.alone:Play(true)
end

gm.lights = {id("light1"),id("light2"),id("light3"),id("light4"),id("light5")}
gm.lcover = {id("cover11"),id("cover12"),id("cover13"),id("cover14"),id("cover15")}
gm.rcover = {id("cover21"),id("cover22"),id("cover23"),id("cover24"),id("cover25")}

function lightColor()
	if fm.raytime:get() and (gm.everlit or gm.lights[5].visible) then
		gm.everlit = true
		if gm.hissing or gm.dissolving then
			for i,v in ipairs(gm.lights) do
				v:setColor(1,0,0,1)
			end			
		elseif true then -- fm.raytick%6 > 1 then
			fm.rayrot = fm.rayrot + 1
			for i,v in ipairs(gm.lights) do
				local wantcolor = (i + fm.rayrot) % 5 + 1;
				if wantcolor < 4 then
					v:setColor(1,1,0,1)
				else
					v:setColor(0,0,0,1)
				end
			end
		end
		fm.raytick = fm.raytick + 1
	end
end

lightColor()

function opendoor(cover)
	for i = #cover, 1, -1 do
		local v = cover[i]
		act:push({"disable", target=v, wait=km.wait})
	end
end

function leftdoor()
	if gm.lopen then
		go("inside")
	end
end

function rightdoor()
	if gm.ropen then
		go("end")
	end
end

function createDissolveScreen()
	local newscreen = Screen()
	newscreen:setScreenShader("DissolveMaterial")
	shaders.dissolve = shaderBindings(newscreen,
		{xd=1/surface_width, yd=1/surface_height, thresh=0.0})	
	screens.dissolve = newscreen
end

act:push({"do", this=function()
	gm.hissing = true
	gm.hissingat = ticks
	au.lotone:Play(false)
	au.hiss:Play(true)
end})

createDissolveScreen()

local newscreen = Screen()
newscreen:setScreenShader("FilterMaterial")
screens.cloud = newscreen
shaders.cloud = shaderBindings(newscreen,
	{radius=(fm.helmet and km.helmetfactor or km.nohelmetfactor)/surface_width/2, aspect=(surface_width/surface_height)})