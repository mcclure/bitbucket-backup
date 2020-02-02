-- Level themeing and appearance

class "LevelTheme"

function LevelTheme:LevelTheme(spec)
	pull(self,
		{})
	pull(self, spec)
end

function LevelTheme:makeBoxWithLetter(x,y,z,letter,w)
	w = w or 0.75
	local box = bridge:uprightBox(w,2.0,w)
	box:setPosition(x, y, z)
	box:setTexture(texForLetter(letter))
	s:addPhysicsChild(box, SHAPE_BOX, 1.0)
	return box
end

function LevelTheme:randomBoxColor(b)
	b.r = math.random() b.g = math.random() b.b = math.random()
end

function LevelTheme:makeWall(wallextent, axis, dir)
	local xaxis = axis == "x"
	local len
	if xaxis then
		wallextent:offset(0.5*dir,0)
		wallextent:inset(0,-0.5)
		len = (wallextent.y2-wallextent.y1)
	else
		wallextent:offset(0,0.5*dir)
		wallextent:inset(-0.5,0)
		len = (wallextent.x2-wallextent.x1)
	end
	
	local center = wallextent:center()
	local wall = bridge:normalPlane(len, 2.5)
	wall:Yaw((xaxis and 90 or 0) + (dir>0 and 0 or 180))
	wall:setPosition( center.x, 2.5/2, center.y )
	s:addPhysicsChild(wall, SHAPE_BOX, 0.0)
	if self.flat then
		wall:setColor(self.flat[1],self.flat[2],self.flat[3],1)
	else
		if self.wallcolor then
			self.wallcolor(wall)
		else
			wall:setColor(1,1,1,1)
		end
		wall:setMaterialByName("GroundMaterial")
	end
	noMovement(wall)
	return wall
end

function LevelTheme:skySet(obj, coloring)
	if #coloring == 2 then
		bridge:setGradient(obj, coloring[1][1],coloring[1][2],coloring[1][3],1, coloring[2][1],coloring[2][2],coloring[2][3],1)
	else
		obj:setColor(coloring[1][1],coloring[1][2],coloring[1][3],1)
	end
end

function LevelTheme:makeSky()
	if self.sky then
		bg:getDefaultCamera():setPosition(0,100,0)
		bg:getDefaultCamera():lookAt(Vector3(0,0,0),Vector3(0,0,1)) 

		local bgup = 84

		local bgtop = ScenePrimitive(TYPE_PLANE, bgup * (surface_width / surface_height), bgup / 2)
		bgtop:loadTexture("media/material/floor_texture.png")
		bgtop:setPosition(0,0,bgup/4)
		self:skySet(bgtop, self.sky[1])
		bg:addEntity(bgtop)

		local bgbot = ScenePrimitive(TYPE_PLANE, bgup * (surface_width / surface_height), bgup / 2)
		bgbot:loadTexture("media/material/floor_texture.png")
		bgbot:setPosition(0,0,-bgup/4)
		self:skySet(bgbot, self.sky[2] or self.sky[1])
		bg:addEntity(bgbot)
	end
end