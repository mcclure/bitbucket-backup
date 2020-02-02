-- On Update

-- Make sure that center of blur effect is always the position of the ball
local p = ball:getPosition()
if p.y > surface_height + ball:getHeight()*2 then
	setPosition(ball, ballstart.x, ballstart.y-ball:getHeight()*2)
end
local center = a(shader.center:get())
center.x = p.x / surface_width
center.y = 1-p.y / surface_height
shader.center:set(center)

-- Click on ball to trigger damped harmonic oscillator
if wobble_v ~= 0 then
	local wobble_a = -wobble_c * 0.1
	wobble_v = wobble_v + wobble_a
	wobble_c = wobble_c + wobble_v	
	wobble_v = wobble_v * 0.9
	
	shader.radius:set( 0.1 * math.exp( wobble_c ) )
	
	-- Also flicker red.
	local ballcolor = math.abs(wobble_v*10)
	ball:setStrokeColor(ballcolor,0,0,1)
	ball[-1]:setColor(ballcolor,0,0,1)
	ball[1]:setColor(ballcolor,0,0,1)
end

-- Click and hold to apply a force on the ball
if down[0] then
	local x = mouseAt.x - p.x
	local y = mouseAt.y - p.y
	PhysicsScreen(screen()):applyForce(ball, x*5, y*5)
	if not humEffect:isPlaying() then
		humEffect:Play(true)
		ball:setColor(0,1,0,1)
	end
else
	if humEffect:isPlaying() then
		humEffect:Stop()
		ball:setColor(1,1,0,1)
	end
end
