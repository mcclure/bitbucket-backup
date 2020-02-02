-- Pregame - On Load

pull(gm, {ls={}})
pull(km, {step=8, redlayer=1, greenlayer=2, bluelayer=4})

function make_image(sprite)
	local name = string.format("media/sprite/%s.png",sprite)
	local g = gfxcontainer()
	local success = g:load_image(name)
	if success then g:pxcopy_invert(g) return r(g) end
	delete(g) return nil
end

function is_wall(e)
	return type(e) == "number"
end

class "Controller" (Ent)

local suffixes1 = {"", "-back", "-left", "-right"}
local suffixes2 = {"", "-w1", "-w2"}

function Controller:Controller(spec)
	pull(self, {pos=p0, offset=p0})
	Ent.Ent(self,spec)
	self.sprites = {}
	for i1,s1 in ipairs(suffixes1) do
		for i2,s2 in ipairs(suffixes2) do
			local keyname = string.format("%d%d", i1, i2)
			local spritename = string.format("%s%s%s", self.basis, s1,s2)
			self.sprites[keyname] = make_image(spritename)
		end
	end
	self.sprite = self.sprites["11"]
end

function Controller:insert()
	Ent.insert(self)
	self.live:load(self)
	return self
end

local dirs = {cursor_left=P(-1,0), cursor_right=P(1,0), cursor_up=P(0,-1), cursor_down=P(0,1)}
local keys = {cursor_left=3, cursor_up=2, cursor_down=1, cursor_right=4}
local flips = {cursor_left=true, cursor_right=false}

function Controller:spriteFor(d, s) -- Direction, "step"
	function key(d,s) return string.format("%d%d", d, s) end
	local sprite = self.sprites[key(d, s)]
	if not sprite then sprite = self.sprites[key(1, s)] end
	if not sprite then sprite = self.sprites[key(d, 1)] end
	if not sprite then sprite = self.sprites[key(1, 1)] end
	return sprite
end

function Controller:complete(act)
	self.sprite = self.sprites["11"]

	self.live:unload(self)
	self.pos = self.pos:add(act.dir)
	self.live:load(self)
	
	self.offset = p0
end

function Controller:process(act)
	self.offset = self.offset:add(act.dir:mult(act.speed))
	self.sprite = self:spriteFor(act.spritekey, math.floor((act.step-1)/4)%2 + 2)
end

function Controller:moveAct(spec)
	local baseSprite = self.sprites["11"]
	local act = tableMerge({to=math.ceil((baseSprite and self.live.cell.x or km.step)/(spec.speed or 1)), step=1, speed=1}, spec)
	if self:moveInto(act) then
		self.act = act
		self:process(self.act)
		return true
	end
	return false
end

function Controller:moveInto(spec)
	local inpos = self.pos:add( spec.dir )
	local into = self.live:collide(inpos.x, inpos.y)
	
	-- Check walls first
	for k,v in pairs(into) do
		if is_wall(v) then
			return false
		end
	end
	
	-- Check entities
	for k,v in pairs(into) do -- No walls or we'd be dead
		if not v:moveAct(spec) then -- Basically assumes no tenant "doubling"?
			return false
		end
	end
	
	return true
end

function Controller:onTick()
	if self.act then
		self:process(self.act)
		self.act.step = self.act.step + 1
		if self.act.step >= self.act.to then
			local newact = self:complete(self.act)
			self.act = newact
		end
	end
	
	local at = self.live:rebase(self.pos)
	self.live.g:pxcopy_blend(self.sprite, at.x+self.offset.x, at.y+self.offset.y, 0, 0, -1, -1, not self.xflip)
end

class "PlayerController" (Controller)

function PlayerController:PlayerController(spec)
	pull(self, {xflip=true, basis="self", pressed={
		cursor_left=self.cursor, cursor_right=self.cursor, cursor_up=self.cursor, cursor_down=self.cursor
	}})
	Controller.Controller(self,spec)
end

function PlayerController:cursor(why)
	if not self.act then
		self:moveAct({dir=dirs[why], spritekey=keys[why], speed=1})
		if flips[why] ~= nil then self.xflip = flips[why] end
	end
end

class "BlockController" (Controller)

function BlockController:BlockController(spec)
	pull(self, {basis="block-push"})
	Controller.Controller(self,spec)
end

class "FixedController" (Controller)

function FixedController:FixedController(spec)
	Controller.Controller(self, spec)
end

function FixedController:moveAct(spec)
	return self.passable
end

class "DoorController" (FixedController)

function DoorController:DoorController(spec)
	pull(self, {basis="block-door"})
	FixedController.FixedController(self, spec)
end

function DoorController:onTick()
	self.dead = false
	if self.watch and self.target then for i,v in ipairs(self.watch) do
		if v.pos:equals(self.target.pos) then
			self.dead = true
		end
	end end
	if self.dead then return end
	FixedController.onTick(self)
end

function DoorController:moveAct(spec)
	return self.passable or self.dead
end

-- Level container

class "LevelScreen"

local ls_id_generator = 1

function LevelScreen:LevelScreen(spec)
	pull(self, {id=ls_id_generator,cell=P(16,16)})
	pull(self, spec)
	self.g = gfxcontainer()
	self.wall = SparseGrid()
	self.tenant = SparseGrid()
	
	ls_id_generator = ls_id_generator + 1
end

function LevelScreen:load(e)
	local t = self.tenant:get(e.pos.x, e.pos.y, {})
	t[e.id] = e
end

function LevelScreen:unload(e)
	local cell = self.tenant:get(e.pos.x,e.pos.y)
	if cell then cell[e.id] = nil end
end

function LevelScreen:collide(x,y)
	local tenants = self.tenant:get(x,y)
	local wall = self.wall:get(x,y)
	return tableMerge({wall}, tenants)
end

function LevelScreen:insert()
--	gm.ls[self.id] = self
	gm.ls[self.channel] = self
	return self
end

function LevelScreen:reblank()
	self.g:pxclear()
	for pos,v in self.wall:iter() do
		local at = self:rebase(pos)
		self.g:pxfill(0xFFFFFFFF, at.x, at.y, self.cell.x, self.cell.y)
	end
end

function LevelScreen:rebase(p)
	return P( (p.x-1)*self.cell.x + 4, (p.y-1)*self.cell.y )
end

function blittext(g, x, y, text, inv, xoff, yoff)
	inv = inv or false 
	xoff = xoff or 0
	yoff = yoff or 0
	if not gm.english then
		gm.english = gfxcontainer()
		gm.english:load_image("media/english.png")
	end
	g:pxcopy_text(gm.english, text, x*8+xoff, y*8+yoff, inv, -1, -1, 7, -1)
end

function blittext_centered(g, x, y, w, text, inv, xoff, yoff)
	if #text > w then text = text:sub(1,w) end
	blittext(g, math.floor(x + (w - #text)/2), y, text, inv, xoff, yoff) 
end