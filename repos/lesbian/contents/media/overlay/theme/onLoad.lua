-- Level themeing and appearance

class "LevelTheme"

function LevelTheme:LevelTheme(spec)
	pull(self,
		{})
	pull(self, spec)
	self.s = scene()
	self.e = SceneEntity()  -- LEAK
	self.s:addEntity(self.e)
end

function LevelTheme:spritePos(e, pos, spec)
	spec = spec or {}
	e:setPosition(pos.x, cm.camera_height_mod, pos.y)
end

function LevelTheme:makeSprite(pos, spec)
	spec = spec or {}
	
	local box = bridge:uprightBox(km.wallh,km.wallh,km.wallh)
	Services.Renderer:setTextureFilteringMode(Renderer.TEX_FILTERING_NEAREST)
	box:loadTexture(spec.sprite)
	self:spritePos(box, pos)
	self.e:addChild(box)
	if _DEBUG then box.custEntityType = "sprite" end
	return box
end

function LevelTheme:setSprite(e, spec)
	e:loadTexture(spec.sprite)
end

function LevelTheme:randomBoxColor(b)
	b.r = math.random() b.g = math.random() b.b = math.random()
end

function LevelTheme:randomObjectColor(e)
	local box = {}
	self:randomBoxColor(box)
	e:setColor(box.r,box.g,box.b,1)
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
	local wall = bridge:normalPlane(len, km.wallh)
	wall:Yaw((xaxis and 90 or 0) + (dir>0 and 0 or 180))
	wall:setPosition( center.x, 2.5/2, center.y )
	self.e:addChild(wall)
	if self.flat then
		wall:setColor(self.flat[1],self.flat[2],self.flat[3],1)
	else
		if self.wallcolor then
			self.wallcolor(wall)
		else
			wall:setColor(1,1,1,1)
			self:randomObjectColor(wall) -- TEST -- REMOVE ME
		end
		wall:setMaterialByName("GroundMaterial")
	end
	if _DEBUG then wall.custEntityType = "wall" end
	return wall
end

-- For debugging -- unused
function LevelTheme:makeFloor(extent, y, dir)
	y = y or 0
	dir = dir or 1
	local center = extent:center()
	local wall = bridge:normalPlane(extent.x2-extent.x1, extent.y2-extent.y1)
	wall:Pitch(90 + (dir>0 and 0 or 180))
	wall:setPosition( center.x, y, center.y )
	self.e:addChild(wall)
	if self.flat then
		wall:setColor(self.flat[1],self.flat[2],self.flat[3],1)
	else
		if self.wallcolor then
			self.wallcolor(wall)
		else
			wall:setColor(1,1,1,1)
			self:randomObjectColor(wall) -- TEST -- REMOVE ME
		end
		wall:setMaterialByName("GroundMaterial")
	end
	if _DEBUG then wall.custEntityType = "floor" end
	return wall
end

function LevelTheme:makeSky()

end	