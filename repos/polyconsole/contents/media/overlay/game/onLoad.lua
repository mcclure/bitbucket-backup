-- On Load

-- Set up "spotlight" effect
screen():setScreenShader("FilterMaterial")

shader = shaderBindings(screen(), 
	{center=a(Vector2(0.1,0.1)), radius=0.1, intensity=100.0, aspect=(surface_width/surface_height)})

ball = ScreenShape(ScreenMesh(ScreenEntity(id("focus"))))
ballstart = r(vDup(ball:getPosition()))
ball.strokeEnabled = true
ball:setStrokeWidth(2)
ball:setStrokeColor(0,0,0,1)

wobble_c = 0
wobble_v = 0

-- Generate "crash" sound
do
	local data = a(NumberArray())
	local limit = (4096*4)-1
	local v
	for i=0,limit do
		if i % 100 == 0 then
			v = math.random()*2-1
		end
		data:push_back(v*((limit-i)/limit))
	end
	soundEffect = r(bridge:soundFromValues(data))
end

-- Generate "on Click" wobble sound
do
	local data = a(NumberArray())
	local limit = (4096*8)-1
	local v
	for i=0,limit do
		if i % 20 == 0 then
			v = math.random()*2-1
		end
		v = v * math.sin(i / 44100.0 * 2*math.pi * 880 / sqrt(2)) * math.sin(i / 4096 * math.pi / 2) * 2
		v = clamp(-1,v,1)
		data:push_back(v*((limit-i)/limit))
	end
	soundEffect2 = r(bridge:soundFromValues(data))
end

-- Generate "hum" sound
do
	local data = a(NumberArray())
	local limit = 4096
	for i=0,limit do
		v = math.sin(i / 44100.0 * 2*math.pi * 220 / sqrt(2)) * math.sin(i / 4096 * math.pi)
		data:push_back(v)
	end
	humEffect = r(bridge:soundFromValues(data))
end

-- Set up "ball" appearance
do
	local s2 = math.sqrt(2) 
	local x = ball:getWidth()/2/s2
	local y = ball:getHeight()/2/s2
	for i = -1,1,2 do
		local l = ScreenLine(Vector3(-x, -y*i, 0), Vector3(x, y*i, 0))
		l:setColor(0,0,0,1)
		l:setLineWidth(2)
		l:setRotation(45)
		ball:addChild(l)
		ball[i] = l
	end
end
