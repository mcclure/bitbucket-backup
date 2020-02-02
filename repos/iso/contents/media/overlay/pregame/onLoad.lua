-- Pregame on load

gm = {onTick = {}, onInput = {}, onInputStack = Stack()}
km = {BLOCK=1, SPRITE=2, TRIGGER=3, ANCHOR=4, climbmax=1, text_timeout_on = 48, textscale = 1}

function pushInput() gm.onInputStack:push(gm.onInput) gm.onInput = {} end
function popInput() local top = gm.onInputStack:pop() if top then gm.onInput = top end end

-- A point. Should go in project_lua? Should integrate with Vector2 somehow?
class "P"

function P:P(x,y)
	self.x = x self.y = y
end

function P:clone()
	return P(self.x,self.y)
end

function P:travel(axis,dir)
	local p2 = self:clone()
	p2[axis] = p2[axis] + dir
	return p2
end

function P:equals(p)
	return p.x == self.x and p.y == self.y
end

function P:floor()
	return P(math.floor(self.x),math.floor(self.y))
end

-- A rectangle. Should go in project_lua?
class "Rect"

function Rect:Rect(x1,x2,y1,y2)
	pull(self, {x1=x1 or 0, x2=x2 or 0, y1=y1 or 0, y2=y2 or 0})
	if self.x2 < self.x1 then local temp = self.x1 self.x1 = self.x2 self.x2 = temp end
	if self.y2 < self.y1 then local temp = self.y1 self.y1 = self.y2 self.y2 = temp end
end

function Rect:center()
	return P( (self.x1 + self.x2) / 2, (self.y1 + self.y2) / 2 )
end

-- Expand rectangle to contain point
function Rect:fit(x,y)
	if x < self.x1 then self.x1 = x end
	if x > self.x2 then self.x2 = x end
	if y < self.y1 then self.y1 = y end
	if y > self.y2 then self.y2 = y end
	return self
end

-- Expand or contract boundaries by margins
function Rect:inset(x,y)
	y = y or x
	self.x1 = self.x1 + x
	self.x2 = self.x2 - x
	self.y1 = self.y1 + y
	self.y2 = self.y2 - y
	return self
end

function Rect:offset(x,y)
	self.x1 = self.x1 + x
	self.x2 = self.x2 + x
	self.y1 = self.y1 + y
	self.y2 = self.y2 + y
end

-- Test if point is in rect
function Rect:collide(x,y)
	return (self.x1 <= x and self.x2 >= x) and (self.y1 <= y and self.y2 >= y)
end

function Rect:rectCollideInternal(r)
	return self:collide(r.x1,r.y1) or self:collide(r.x2,r.y1) or self:collide(r.x1,r.y2) or self:collide(r.x2,r.y2)
end

-- Test if rect overlaps with other rect
function Rect:rectCollide(r)
	return self:rectCollideInternal(r) or r:rectCollideInternal(self)
end

-- Clone
function Rect:clone()
	return Rect(self.x1, self.x2, self.y1, self.y2)
end

-- A sparse 2D grid. Should go in project_lua?
class "SparseGrid"

function SparseGrid:SparseGrid()
	self.data = {}
end

function SparseGrid:set(x, y, value)
	if not self.data[x] then self.data[x] = {} end
	self.data[x][y] = value
end
function SparseGrid:get(x, y, default)
	local xv = self.data[x]
	local yv = xv and xv[y]
	if not yv and default then self:set(x,y,default) return default end
	return yv
end

-- Returns an iterator that returns P(x,y),value for every point in grid
function SparseGrid:iter()
	local ix, x = nil, nil
	local iy, y = nil, nil
	return function()
		if x then iy, y = next(x, iy) end
		while not iy do 
			ix, x = next(self.data, ix)
			if not ix then return nil end
			iy, y = next(x)
		end
		return P(ix, iy), y
	end
end

-- Returns Rect covering all existing points in grid
function SparseGrid:bounds()
	local r = nil
	for k,v in self:iter() do
		local x = k.x local y = k.y
		if not r then r = Rect(x,x,y,y) else r:fit(x,y) end
	end
	return r
end

-- Sparse grid clone
function SparseGrid:clone()
	local r = SparseGrid()
	for k,v in self:iter() do
		local x = k.x local y = k.y
		r:set(x,y,v)
	end
	return r
end

-- Returns string w/all grid values -- "." for empty -- assumes values are 1-char strings
function SparseGrid:str()
	local bounds = self:bounds()
	local r = "\n"
	for y = bounds.y1, bounds.y2 do
		for x = bounds.x1, bounds.x2 do
			local cell = self:get(x,y)
			local ct = type(cell)
			if ct == "table" then cell = "-"
			elseif cell and ct ~= "string" then cell = tostring(cell) end
			r = r .. (cell or ".")
		end
		r = r .. "\n"
	end
	return r
end

function GridFrom(path)
	local from = bridge:filedump(path)
	local x,y = 0,1
	local g = SparseGrid()
	for c in from:gmatch(".") do
		x = x + 1
		if c == "\n" then
			y = y + 1
			x = 0
		elseif c ~= " " then
			g:set(x,y,c)
		end
	end
	return g
end

class "Level"
-- camera machine: x,y,z is where camera is pointing. xc,yc,zc is "offset from pointing"
function Level:Level(spec)
	pull(self, {own={},cm={x=0,y=0,z=0,xcoff=-1,ycoff=0,zcoff=-1},meta = {},anchors={}})
	pull(self,spec)
	self.map = self.map or SparseGrid()
	if not self.s then 
		self.s = Scene()
	end
	self.theme = self.theme or LevelTheme({s=self.s, cm=self.cm})
	self:updateCamera()
end

function Level:recenterCamera()
	local c = self.map:bounds():center()
	self.cm.x, self.cm.z = c.x, c.y
end
local upright = r(Vector3(0,1,0))
function Level:updateCamera() -- Put in theme!
	local camera = self.s:getDefaultCamera()
	local at = a(Vector3(self.cm.x + self.cm.xcoff, self.cm.y + self.cm.ycoff, self.cm.z + self.cm.zcoff))
	local offs = a(Vector3(10,10,10))
	local at2 = vAdd(at, offs)
	
	local zoom = 8*(self.cm.zoom or 1)
	
	if self.bgimage then killScreen(self.bgimage) delete(self.bgimage) self.bgimage = nil end
	if self.meta.bgimage then
		self.bgimage = self.theme:makeBackground({texture=self.meta.bgimage, at=at2, offs=offs, screenzoom=zoom, texturezoom=self.meta.bgzoom, texturemode=self.meta.bgmode})
	end
	
	for i,camera in ipairs({self.s:getDefaultCamera()}) do
		camera:setOrthoMode(true, zoom*surface_width/surface_height, zoom)
		camera:setPosition(at2.x,at2.y,at2.z)
		camera:lookAt(at,upright)
	end
end

function skim(t, keys)
	local result = {}
	for i,v in ipairs(keys) do
		result[v] = t[v]
	end
	return result
end

function Level:createBlock(spec)
	local obj = Ent(tableMerge({kind=km.BLOCK},spec))
	self:add(obj, spec.x, spec.y, self.theme:makeBox(spec))

	if 1 or _DEBUG then obj.make = tableMerge({kind=obj.kind}, spec) end -- Note: anyone who's messed with make gets stomped
	return obj
end

function Level:createSpecial(spec)
	local obj = Ent(tableMerge({kind=km.TRIGGER},spec))
	self:add(obj, spec.x, spec.y)
	
	if obj.kind == km.ANCHOR then
		self.anchors[obj.anchor] = spec
	end
	
	if 1 or _DEBUG then obj.make = tableMerge({kind=obj.kind}, spec) end -- Note: anyone who's messed with make gets stomped
	return obj
end

-- x, y, flip, etc
function Level:createSprite(spec,obj)
	name = string.format("media/sprite/%s.png", spec.sprite)

	obj = obj or Ent(tableMerge({kind=km.SPRITE},spec))
	self:add(obj, spec.x, spec.y, self.theme:makeSprite({texture=name,color=spec.color}))
	self:spritePos({e=obj.e,x=spec.x,y=spec.y})
	if spec.flip then obj:setFlip(spec.flip) end
	
	if 1 or _DEBUG then obj.make = tableMerge({kind=obj.kind}, spec) end -- Note: anyone who's messed with make gets stomped
	return obj
end

function Level:spritePos(spec)
	local x,y = spec.x,spec.y
	local square = self:find(x,y,km.BLOCK)
	local outspec = {x=x,y=y}
	if square then outspec.height = square.height end
	self.theme:spritePos(spec.e, outspec)
end

function Level:add(obj, x, y, entity)
	if entity then obj.e = entity end
	
	if x and y then
		self.map:get(x,y,{})[obj.id] = obj
	else
		self.own[obj.id] = obj
	end
	return obj
end

function Level:clear(x,y, kind)
	local current = self.map:get(x,y)
	if current then
		for k,v in pairs(current) do
			if not kind or v.kind == kind then
				v:die()
			end
		end
		self.map:set(x,y,{})
	end
end

function Level:find(x,y,kind)
	local current = self.map:get(x,y)
	if current then
		for k,v in pairs(current) do
			if not kind or v.kind==kind then return v end
		end
	end
	return nil
end

-- Resets meta properties not room itself
function Level:reset()
	if self.meta.bgcolor then
		bridge:setSceneClearColor(self.s,self.meta.bgcolor[1]*255,self.meta.bgcolor[2]*255,self.meta.bgcolor[3]*255,1)
	else
		self.s.useClearColor = false
	end
	if self.meta.camat then
		self.cm.x,self.cm.z = self.meta.camat.x, self.meta.camat.y
	else
		self:recenterCamera()
	end
	self.cm.zoom = self.meta.camzoom
	self:updateCamera()
end

function Level:reset_music()
	if _DEBUG and fm.block_music then if fm.musicobj then fm.musicobj:Stop() end return end
	
	if fm.musicplaying ~= gm.level.meta.music then
		if not gm.level.meta.music then return end
		
		fm.musicplaying = gm.level.meta.music
		if fm.musicobj then fm.musicobj:Stop() delete(fm.musicobj) fm.musicobj = nil end
		fm.musicobj = Sound(SoundFormat(fm.musicplaying))
		fm.musicobj:Play(true)
	end
end

function Level:die()
	if self.dead then return self end -- YOLO
	self.dead = true
	if self.cursor then self.cursor:die() end -- Kludge?
	for k,v in self.map:iter() do
		for k2,v2 in pairs(v) do
			v2:die()
		end
	end
	for i,v in ipairs(self.own) do
		v:die()
	end
	if self.bgimage then killScreen(self.bgimage) end
	cdelete(self.s)
	return self
end

local ent_id_generator = 1

class "Ent"
function Ent:Ent(spec)
	pull(self, {id=ent_id_generator,})
	pull(self, spec)
	for i,v in ipairs({"pressed","down"}) do
		if self[v] then 
			if not self.actions then self.actions = {} end
			if not self.actions[v] then self.actions[v] = {} end
			pull(self.actions[v], self[v])
			self[v] = nil
		end
	end
	
	ent_id_generator = ent_id_generator + 1
end

function Ent:insert()
	if self.onInput then gm.onInput[self.id] = self end
	if self.onTick then gm.onTick[self.id] = self end
	if self.actions then
		for kind, kindt in pairs(self.actions) do
			for action, func in pairs(kindt) do
				gm.r:register(kind, action, self, func)
			end
		end
		self.keys = nil
	end
	return self
end

function killScreen(e)
	if e then e:getParentEntity():removeChild(e) end
end

function Ent:die()
	if self.onInput then
		gm.onInput[self.id] = nil
		for i=1,gm.onInputStack.count do
			gm.onInputStack[i][self.id] = nil -- Every part of you must die
		end
	end
	if self.onTick then gm.onTick[self.id] = nil end
	gm.r:unregister(self)
	killScreen(self.e)
	return self
end

function Ent:setFlip(flip) -- Only relevant to entity-owning ents.
	if self.e then
		self.e:setScale(flip and -1 or 1,1,1) -- TODO
	end
	return self
end

function Ent:setListening(v) -- Only relevant to "autonomous" ents.
	self.listener = v and Queue() or nil
	listeners[self.id] = self.listener
end

function Ent:passable()
	return self.kind ~= km.SPRITE
end

-- TODO: Make an Ent? Would maybe require priorities for onTick/onInput
class "Router" (Ent)

function Router:Router(spec)
	Ent.Ent(self,spec)
	if not self.route then
		self.route = { pressed={}, down={} }
	end
end

function Router:register(kind, action, obj, func)	
	local key = keymap[action]
	if not key then return end
	local kt = self.route[kind]
	if not kt then return end
	if not kt[key] then kt[key] = {} end
	kt[key][obj.id] = {obj, func, action}
end

function Router:unregister(obj)
	local id = obj.id
	for _,kindt in pairs(self.route) do
		local collect = {}
		for key, keyt in pairs(kindt) do
			keyt[id] = nil
			if not tableTrue(keyt) then collect[key] = true	end
		end
		for i,key in ipairs(collect) do
			kindt[key] = nil
		end
	end
end

function Router:execute(keyt)
	for i,v in pairs(keyt) do
		local obj, func, action = unpack(v)
		func(obj, action)
	end
end

function Router:onInput()
	for key, keyt in pairs(self.route.pressed) do
		if pressed[key] then self:execute(keyt) end
	end
	for key, keyt in pairs(self.route.down) do
		if pressed[key] then self:execute(keyt) end	
	end
end

class "Controller" (Ent)
function Controller:Controller(spec)
	Ent.Ent(self,spec)
	if self.make then
		local make = self.make
		self.level = make.level or gm.level
		if make.sprite then self.level:createSprite(make, self) self.sprite = true end
		self.x, self.y = make.x, make.y
		if not (1 or _DEBUG) then
			self.make = nil
		end
	end
end

function Controller:at() return P(self.x,self.y) end
function Controller:set_at(at) self.x = at.x self.y = at.y end

-- NW NE SW SE = 0 1 2 3
function Controller:move(dir)
	self.facedown = dir>1 -- "South" (dubious?)
	self.faceleft = 0==dir%2 -- "East" (dubious?)
	local axis = (dir==1 or dir == 2) and "x" or "y"
	local mag = 0==dir%2 and -1 or 1
	
	local try = self:at()
	try[axis] = try[axis] + mag
	if self:safe(try.x,try.y) then
		self:set_at(try)
		
		if self.sprite then
			self.level:spritePos(self)
		else
			self.level.theme:objPos(self.e, self)
		end
	end
	self:setFlip(not self.facedown)
	if self.make.backsprite then
		self.e:loadTexture( string.format("media/sprite/%s.png", self.faceleft and self.make.backsprite or self.make.sprite) )
	end
end

function Controller:safe(x,y) return true end

function Controller:move_nw() self:move(0) end
function Controller:move_ne() self:move(1) end
function Controller:move_sw() self:move(2) end
function Controller:move_se() self:move(3) end

class "PlayerController" (Controller)
function PlayerController:PlayerController(_spec)
	local spec = tableCopy(_spec)
	spec.pressed = tableMerge({move_sw=self.move_sw,move_se=self.move_se,move_nw=self.move_nw,move_ne=self.move_ne}, spec.pressed)
	spec.make =    tableMerge({sprite="trusprit/dudemanwalkin2", backsprite="trusprit/dudemanwalkin",	special="player",}, spec.make)
	Controller.Controller(self,spec)
end

function PlayerController:checkBlock(trigger,p)
	local passable = trigger:passable()
	if trigger.warpto then
		loadnext = trigger.warpto
		loadnextanchor = trigger.warpanchor
	end
	local act = trigger.act and script[trigger.act]
	if act then
		if type(act) == "string" then
			dialog_text(act)
			return passable
		elseif type(act) == "function" then
			return act(trigger,self,p)
		else
			return act:act(trigger,self,p)
		end
	end
	return passable
end

function PlayerController:safe(x,y)
	local p = P(x,y)
	local block = self.level:find(x,y,km.BLOCK)
	if not block then return false end
	
	local currentblock = self.level:find(self.x,self.y,km.BLOCK)
	if currentblock and math.abs((block.make.height or 0)-(currentblock.make.height or 0)) > km.climbmax then return false end
	
	local sprite = self.level:find(x,y,km.SPRITE)
	if sprite then return self:checkBlock(sprite,p) end
	
	local trigger = self.level:find(x,y,km.TRIGGER)
	if trigger then return self:checkBlock(trigger,p) end
	
	return true
end

function PlayerController:move(dir)
	finished_dialog_text()
	Controller.move(self,dir)
end

class "Eater" (Ent)
function Eater:Eater(spec)
	Ent.Ent(self, spec)
end

local termcolor = normalColor(1,1,0,1)

function Eater:insert()
	pushInput()
	Ent.insert(self)
	bridge:term_setOverride(true)
	bridge:term_setEntry(self.message)
	if self.topmessage then
		for i,v in ipairs(self.topmessage) do
			bridge:term_setLine(i-1,v)
			bridge:term_setColor(i-1,termcolor)
		end
	end
	return self
end

function Eater:die()
	Ent.die(self) -- Unnecessary?
	popInput()
	bridge:term_setOverride(false)
end

class "NumberEater" (Eater)
function NumberEater:NumberEater(spec)
	Eater.Eater(self, spec)
	spec.message = spec.message or "Enter a number:"
end

function NumberEater:onInput(spec)
	for i=KEY_0,KEY_9 do
		if pressed[i] then
			self:die()
			local delegateFunction = self.delegateFunction or self.delegate.onNumber
			if self.delegate then delegateFunction(self.delegate, i-KEY_0) end
			break
		end
	end
end

class "StringEater" (Eater)
function StringEater:StringEater(spec)
	Eater.Eater(self, spec)
	self.message = self.message or "Enter: "
	self.input = Stack()
	if self.seed then
		for c in self.seed:gmatch(".") do
			self.input:push(c)
		end
		self.seed = nil
	end
end

function StringEater:insert()
	Eater.insert(self)
	self:setListening(true)
	bridge:term_setEntry(self.message .. self:contents())
	return self
end

function StringEater:die()
	self:setListening(false)
	Eater.die(self)
end

local ascii_space = string.byte(" ")
local ascii_tilde = string.byte("~")
function isAlpha(v)
	if not v then return false end
	local ascii = string.byte(v)
	return ascii and (ascii >= ascii_space) and (ascii <= ascii_tilde)
end

function StringEater:contents()
	local message = ""
	for i=1,self.input.count do -- DRY
		local v = self.input[i]
		message = message .. v
	end
	return message
end

function StringEater:onInput(spec)
	local rewrite = false
	while self.listener do
		local t = self.listener:pop()
		if not t then break end
		local k,v = unpack(t)
		if k == KEY_RETURN or k==KEY_ENTER then
			local delegateFunction = self.delegateFunction or self.delegate.onNumber
			local message = self:contents()
			
			self:die()
			
			if self.delegate and (self.emptyok or #message > 0) then delegateFunction(self.delegate, message) end
			return
		elseif k == KEY_BACKSPACE then
			self.input:pop()
			rewrite = true
		elseif isAlpha(v) then
			self.input:push(v)
			rewrite = true
		end
	end
	
	if rewrite then
		bridge:term_setEntry(self.message .. self:contents())
	end
end

-- TRIGGERS

function dialog_text(str)
	gm.textfield:setText(str)
	local w = bridge:labelWidth(gm.textfield)*km.textscale
	local h = bridge:labelHeight(gm.textfield)*km.textscale
	gm.textfield:setPosition((surface_width-w)/2,h/2)
	gm.textback.visible = true
	gm.havetext = true
	gm.text_timeout = nil
end

function finished_dialog_text()
	if not gm.text_timeout then 
		gm.text_timeout = ticks
	end
end

function clear_dialog_text()
	if gm.havetext then
		gm.textfield:setText("")
		gm.havetext = false
		gm.textback.visible = false
	end
end

function recenter_camera(trigger,player,p)
	gm.level.cm.x, gm.level.cm.z = p.x, p.y
	gm.level:updateCamera()
	return true
end

function endgame()
	loadroomnext = "media/outro.txt"
end

sm = sm or {}
function SoundFormat(sound)
	return string.format("media/ogg/%s.ogg",sound)
end
function SoundReady(sound)
	if not sm[sound] then
		sm[sound] = Sound(SoundFormat(sound))
	end
	return sm[sound]
end

class "SoundAct"
function SoundAct:SoundAct(sound)
	self.sound = sound
	self.soundobj = SoundReady(self.sound)
end
function SoundAct:act(trigger)
	self.soundobj:Play(false)
	return trigger:passable()
end

class "ZoomAct"
function ZoomAct:ZoomAct(zoom)
	self.zoom = zoom
end
function ZoomAct:act(trigger)
	gm.level.cm.zoom = self.zoom
	gm.level:updateCamera()
	return trigger:passable()
end

class "ShaderAct"
function ShaderAct:ShaderAct(shader, content)
	
end