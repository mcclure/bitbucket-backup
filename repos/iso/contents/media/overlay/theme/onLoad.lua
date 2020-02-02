-- Level theming and appearance

tkm = {cellwidth = 1}

function normalColor(r,g,b,a) local c=Color() c:setColorRGBA(r*255,g*255,b*255,a*255) return c end

class "LevelTheme"

-- Spec needs: s, cm
function LevelTheme:LevelTheme(spec)
	pull(self,
		{})
	pull(self, spec)
	if not self.e then
		self.e = SceneEntity()
		self.s:addEntity(self.e)
		
		self.bge = SceneEntity()
		local bg = self.bg or self.s
		bg:addEntity(self.bge)
	end
end

local defaultBoxColor = {1,1,1}

function textureLoad(dir, name, clamp)
	local ci = clamp and 1 or 0
	if not fm.texture_cache then fm.texture_cache = {} end
	if not fm.texture_cache[ci] then fm.texture_cache[ci] = {} end
	if not fm.texture_cache[ci][name] then
		local fullname = string.format("media/%s/%s.png", dir, name)
		local image = a(Image(fullname))
		fm.texture_cache[ci][name] = Services.MaterialManager:createTextureFromImage(image, clamp, false)
	end
	return fm.texture_cache[ci][name]
end

function LevelTheme:makeBackground(_spec)
	spec = {screenzoom=1, texturezoom=1, texturemode=1}
	pull(spec, _spec)
	local s = self.bg or self.s
	if spec.texturemode < 1 or spec.texturemode > 5 then spec.texturemode = 1 end
	local tiling = spec.texturemode == 4 or spec.texturemode == 5
	local fillscreen = spec.texturemode ~= 1

	Services.Renderer:setTextureFilteringMode(Renderer.TEX_FILTERING_NEAREST)
	local image = textureLoad("bg",spec.texture, not tiling)
	local width, height = image:getWidth(), image:getHeight()
	local scale = spec.screenzoom
	local inscale = 1.0
	
	if tiling then
		inscale = inscale * spec.texturezoom
	else
		scale = scale * spec.texturezoom
	end
	
	if spec.texturemode == 4 then -- Results tile 1:1
		inscale = inscale * surface_height / height
	end
	
	local hscale = scale
	local hinscale = inscale
	if fillscreen then
		local mod = surface_width/surface_height
		hscale = hscale * mod
		if spec.texturemode == 2 then
			scale = hscale -- Results fill leftright
		elseif spec.texturemode ~= 3 then
			hinscale = hinscale * mod -- Results in stretching inside texture
		end
	end
	
	local box = bridge:normalPlane(hscale, scale, hinscale, inscale, true)
	box.alphaTest = true
	box.billboardMode = true
	box.backfaceCulled = false
	box:setTexture(image)
	
	local at3 = vAdd(spec.at, vMult(spec.offs, -5))
	vSetPosition(box, at3)
	
	self.bge:addChild(box)
	
	return box
end

function LevelTheme:makeBox(_spec)
	spec = {texture="plain", height=0, color=defaultBoxColor}
	pull(spec, _spec)
	local height = (spec.height or 0)/2
	local pos = vMult(a(Vector3(spec.x, height/2, spec.y)), tkm.cellwidth)
	
	local box = bridge:uprightBox(tkm.cellwidth,height+0.5,tkm.cellwidth, spec.textype == 3)
	Services.Renderer:setTextureFilteringMode(Renderer.TEX_FILTERING_NEAREST)
	box:setTexture(textureLoad("tile",spec.texture, (spec.textype ~= 2)))
	vSetPosition(box, pos)
	box:setColor(spec.color[1],spec.color[2],spec.color[3],1)
	self.e:addChild(box)
	return box
end

function LevelTheme:objPos(box, spec)
	local pos = vMult(a(Vector3(spec.x, 1, spec.y)), tkm.cellwidth)
	vSetPosition(box, pos)
end

function LevelTheme:spritePos(box, spec)
	local z = (spec.height and spec.height/2 or 0) + 1
	local pos = vMult(a(Vector3(spec.x + 1/3, z, spec.y + 1/3)), tkm.cellwidth)
	vSetPosition(box, pos)
end

function LevelTheme:makeSprite(_spec)
	spec = {width=tkm.cellwidth, height=tkm.cellwidth}
	pull(spec, _spec)

	local box = ScenePrimitive(ScenePrimitive.TYPE_VPLANE, spec.width, spec.height)
	Services.Renderer:setTextureFilteringMode(Renderer.TEX_FILTERING_NEAREST)
	box.alphaTest = true
	box.billboardMode = true
	box.backfaceCulled = false
	box:loadTexture(spec.texture)
	if spec.color then box:setColor(spec.color[1],spec.color[2],spec.color[3],1) end
	self.e:addChild(box)
	return box
end

local defaultCursorColor = {1,1,0}

function LevelTheme:makeCursor(_spec)
	spec = {width=tkm.cellwidth, height=20, color=defaultCursorColor}
	pull(spec, _spec)

	local box = ScenePrimitive(ScenePrimitive.TYPE_BOX, spec.width, spec.height, spec.width)
	box:setColor(spec.color[1],spec.color[2],spec.color[3],1)
	box.backfaceCulled = false
	self:objPos(box, spec)
	self.e:addChild(box)
	return box
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
	local s = scene()
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
	s:addEntity(wall)
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
	return wall
end

function LevelTheme:makeSky()

end