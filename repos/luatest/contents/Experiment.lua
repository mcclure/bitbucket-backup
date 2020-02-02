class "BrokenAsteroids" (EventHandler)

function BrokenAsteroids:BrokenAsteroids(player)
        self.shootSound = Sound("collision.wav")
		self.burstSound = Sound("collision.wav")
		self.burstSound:setPitch(0.5)
		self.burstSound:setVolume(2.0)
		self.deathSound = Sound("collision.wav")
		self.deathSound:setPitch(0.125)
		self.deathSound:setVolume(10000.0)
		self.player = player
		self.dx = 0
		self.dy = 0
		self.dead = false
		self.mousepos = Vector2(0,0)
        self:EventHandler()
end

function BrokenAsteroids:movePlayer(elapsed)
		if self.dead then return end
		local cpos = self.player:getPosition()
		local px = cpos.x - self.mousepos.x -- want vector ops!!
		local py = cpos.y - self.mousepos.y
		local theta = math.atan2(py,px)
		cpos.x = cpos.x + self.dx
		cpos.y = cpos.y + self.dy
		screen:setTransform(self.player,cpos,math.deg(theta)-90) -- Why 90?		
end

function BrokenAsteroids:handleEvent(e)
        if e:getDispatcher() == screen then
                local pe = PhysicsScreenEvent(e)
                if e:getEventCode() == EVENT_NEW_SHAPE_COLLISION then
						local first = pe:getFirstEntity()
						local second = pe:getSecondEntity()
						if first.type > second.type then
							local temp = second; second = first; first = temp
						end
						if (first.type == 1 and second.type == 2) then
							self.deathSound:Play()
							self.dead = true
							table.remove(wrappers, 0)
							table.insert(dyingbullet, self.player)
						elseif (first.type == 2 and second.type == 3) then -- Block vs Bullet
							table.remove(wrappers,second.removeat)
							table.insert(dyingbullet,second)
							
							table.insert(dyingblock,first)
							self.burstSound:Play()
							score = score + 100
                        end
                        --if pe.impactStrength > 5 then
                end
        end
		if e:getDispatcher() == Services.Core:getInput() then
                local inputEvent = InputEvent(e)
                local key = inputEvent:keyCode()
				if self.dead then return end
                if e:getEventCode() == EVENT_KEYDOWN then
                        if key == KEY_LEFT or key == KEY_a then
                                self.dx = -2
                        elseif key == KEY_RIGHT or key == KEY_d then
                                self.dx = 2
                        end
						if key == KEY_UP or key == KEY_w then
                                self.dy = -2
                        elseif key == KEY_DOWN or key == KEY_s then
                                self.dy = 2
                        end
                elseif e:getEventCode() == EVENT_KEYUP then
                        if key == KEY_LEFT or key == KEY_RIGHT or key == KEY_a or key == KEY_d then
                                self.dx = 0
                        end
						if key == KEY_UP or key == KEY_DOWN or key == KEY_w or key == KEY_s then
                                self.dy = 0
                        end
                elseif e:getEventCode() == EVENT_MOUSEMOVE then
                        local pos = inputEvent:getMousePosition()
                        self.mousepos.x = pos.x
						self.mousepos.y = pos.y
                        delete(pos)
                elseif e:getEventCode() == EVENT_MOUSEDOWN then
						self.shootSound:Play()
						self:makeBullet()
                end
        end

end

function BrokenAsteroids:makeAsteroid(isBrandNew)
		shape = ScreenImage("rock" .. math.random(9) .. ".png")
		shape:setRotation(random(360))
		if isBrandNew then
			shape:setPosition(random(640), random(300))
		end
		shape.type = 2
        screen:addPhysicsChild(shape, ENTITY_RECT, false)
		if isBrandNew then
		screen:setVelocity(shape, random(16)-8, random(16)-8)
		end
		table.insert(wrappers, shape)
		return shape
end

function BrokenAsteroids:makeBullet()
		local existingBullets = #dyingbullet
		local shape
		local cpos = self.player:getPosition()
		local crot = math.rad(self.player:getRotation()) - math.pi/2
		local vx = cos(crot)
		local vy = sin(crot)
		
		if existingBullets > 0 then
			shape = dyingbullet[existingBullets]
			table.remove(dyingbullet, existingBullets)
		else
			shape = ScreenShape(SHAPE_RECT, 5,5)
			shape.type = 3
			screen:addPhysicsChild(shape, ENTITY_RECT, false, 0.1, 1, 0, false, true)
		end
		local p = shape:getPosition()
		p.x = cpos.x + vx*30
		p.y = cpos.y + vy*30
		screen:setTransform(shape, p, 0)
		screen:setVelocity(shape, vx*30, vy*30)
		shape.removeat = #wrappers
		table.insert(wrappers, shape)
end

function BrokenAsteroids:makePlayer()
	local shape = ScreenImage("ship.png")
	shape:setPositionMode(POSITION_CENTER)
	shape:setPosition(640/2, 400)
	shape.type = 1
	screen:addPhysicsChild(shape, ENTITY_RECT, false)
	self.player = shape
	table.insert(wrappers, shape)
end
screen = PhysicsScreen(10, 60)
screen:setGravity( Vector2( 0,0 ) )

wrappers = {}
dyingbullet = {}
dyingblock = {}
table.insert(wrappers, shape)

score = 0
scoreChanged = true
scoreBoard = ScreenLabel("", 32)
screen:addChild(scoreBoard)

brokenAsteroids = BrokenAsteroids()

brokenAsteroids:makePlayer()
for i=0,4 do
        brokenAsteroids:makeAsteroid(true)
end

screen:addEventListener(brokenAsteroids, EVENT_NEW_SHAPE_COLLISION)

Services.Core:getInput():addEventListener(brokenAsteroids, EVENT_KEYDOWN)
Services.Core:getInput():addEventListener(brokenAsteroids, EVENT_KEYUP)
Services.Core:getInput():addEventListener(brokenAsteroids, EVENT_MOUSEMOVE)
Services.Core:getInput():addEventListener(brokenAsteroids, EVENT_MOUSEDOWN)

function Update(elapsed)
		if scoreChanged then
			scoreBoard:setText("Score: " .. score .. (brokenAsteroids.dead and "     GAME OVER" or ""))
		end
        brokenAsteroids:movePlayer(elapsed)
		for i,wrapper in ipairs(wrappers) do
			local p = wrapper:getPosition()
			local changed = false;
			if p.x < 0 then p.x = p.x + 640; changed=true end
			if p.x > 640 then p.x = p.x - 640; changed=true end
			if p.y < 0 then p.y = p.y + 480; changed=true end
			if p.y > 480 then p.y = p.y - 480; changed=true end
			if changed then screen:setTransform(wrapper,p,wrapper:getRotation()) end
		end
		for i,bullet in ipairs(dyingbullet) do -- Because we can't remove objects from the space.
			local p = bullet:getPosition()
			p.x = 1000 + random(1000)
			p.y = 1000 + random(1000)
			screen:setTransform(bullet, p, 0)
			screen:setVelocity(bullet, 0, 0)
		end
		for i,block in ipairs(dyingblock) do
			local v = screen:getVelocity(block)
			local p = block:getPosition()
			screen:applyImpulse(block, v.y*100, -v.x*100) -- Does this even do anything?
			local newblock = brokenAsteroids:makeAsteroid(false)
			screen:setTransform(newblock, p, newblock:getRotation())
			screen:setVelocity(newblock, -v.y, v.x);
		end
		dyingblock = {}
end