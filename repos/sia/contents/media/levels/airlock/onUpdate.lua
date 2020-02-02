local circlesize = 50/600 * surface_height
local hlmax = 60*3
local dlmax = 60*2

if gm.hissing then
	local hl = ticks - gm.hissingat
	local overload = hl > hlmax*0.75
	gm.pitcharg = 0.1 + (hl/hlmax)*(0.5-0.1)
	
	for i=1,(overload and 10 or (fm.helmet and 4 or 5)) do
		local shape = ScreenShape(SHAPE_CIRCLE, circlesize,circlesize, 30)
		local y = math.random()
		shape:setPosition(math.random(0,surface_width),(1-math.pow(y,math.sqrt(2.5)))*surface_height)
		shape:setColor(1.0, 1.0, 1.0, overload and 0.9 or 0.8)
		screens["cloud"]:addChild(shape)
	end
	
	if hl >= hlmax then	
		gm.hissing = false
		au.hiss:Stop()
		
		local img = Services.Renderer:renderScreenToImage()

		removeScreen("cloud")
		
		shaders.dissolve.thresh:set(1)
		
		local simg = bridge:screenImageWithImage(img)
		screens.dissolve:addChild(simg)

		if fm.helmet then
			-- SURVIVE
			fm.helmet = false
			fm.sublimated_helmet = true
			shaders.airlock.radius:set(km.nohelmetfactor/surface_width/2)
		elseif fm.sublimated_helmet then
			fm.helmet = true
			fm.sublimated_helmet = false
			shaders.airlock.radius:set(km.helmetfactor/surface_width/2)
			gm.fromright = true
		else
			-- DIE
			gm.dying = true
			removeScreen("airlock")
		end

		if not gm.dying then gm.pitcharg = nil end
		gm.dissolving = true
		gm.dissolvingat = ticks
	end
end

if gm.dissolving then
	local dl = ticks - gm.dissolvingat
	local progress = dl/dlmax
	shaders.dissolve.thresh:set(1-(progress))
	
	if gm.dying then gm.thresharg = 1-progress end
	
	if dl >= dlmax then
		removeScreen("dissolve")
		if gm.dying then
			go("dead")
		else
			au.hitone:Play(false)
			gm.dissolving = false

			if gm.fromright then
				opendoor(gm.lcover)
				gm.lopen = true
			else
				opendoor(gm.rcover)
				gm.ropen = true
			end
		end
	end
end

alone_pitch(gm.pitcharg, gm.thresharg)
lightColor()