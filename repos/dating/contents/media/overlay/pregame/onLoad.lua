-- Pregame - On Load

pull(gm, {})
pull(km, {step=8, 
	cell=P(32,32), lw=9, lh=6
})

function make_image(sprite, context)
	local name = string.format("media/%s/%s.png", context or "sprite", sprite)
	local g = gfxcontainer()
	local success = g:load_image(name)
	if success then return r(g) end
	delete(g) return nil
end

class "Controller" (Ent)

local suffixes1 = {"", "-back", "-left", "-right"}
local suffixes2 = {"", "-w1", "-w2", "-w3"}

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

	if act[1]=="m" then
		self.live:unload(self)
		self.pos = self.pos:add(act.dir)
		self.live:load(self)
		
		self.offset = p0
	end
end

function Controller:process(act)
	if act.dir then self.offset = self.offset:add(act.dir:mult(act.speed)) end
--	print({act.step, math.floor((act.step-1)/6)%3 + 2})
	self.sprite = self:spriteFor(act.spritekey, math.floor((act.step-1)/4)%3 + 2)
end

function Controller:moveAct(spec)
	local baseSprite = self.sprites["11"]
	local act = tableMerge({to=math.ceil((baseSprite and km.cell.x or km.step)/(spec.speed or 1))+1, step=1, speed=1}, spec)
	if self:moveInto(act) then
		self.act = act
		return true
	end
	return false
end

function Controller:moveInto(spec)
	local inpos = self.pos:add( spec.dir )

	-- No collision check just edge test.
	if inpos.x < 1 or inpos.x > km.lw then return false end
	
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
end

function Controller:draw()
	if self.invisible then return end
	local at = self.live:rebase(self.pos)
	self.live.g:pxcopy_blend(self.sprite, at.x+self.offset.x, at.y+self.offset.y, 0, 0, -1, -1, self.xflip)
end

class "PlayerController" (Controller)

function PlayerController:PlayerController(spec)
	pull(self, {basis="self", pressed={
		cursor_left=self.cursor, cursor_right=self.cursor, cursor_up=self.cursor, cursor_down=self.cursor,
		hammer=self.hammer,
	}})
	Controller.Controller(self,spec)
end

function PlayerController:actfor(why)
	return {"m", dir=dirs[why], spritekey=keys[why], speed=2, why=why}
end

function PlayerController:cursor(why)
	if self.mute then return end
	if not self.act then
		self:moveAct(self:actfor(why))
		if flips[why] ~= nil then self.xflip = flips[why] end
	else
		if not storedwhy or why ~= self.act.why then self.storedwhy = why end
	end
end

function PlayerController:hammer(why)
	if self.mute then return end
	if not self.act then
		self.act = {"h", to=km.cell.x/2+1, step=1, spritekey=keys.cursor_up, why=why}
		
		local slam = gm.ls:collide(self.pos.x, self.pos.y)[1]
		if slam and slam.onTap then slam:onTap() end
	else
		if self.act.why == why or (self.act.step/self.act.to)>0.2 then
			self.storedwhy = why
		end
	end
end

function PlayerController:complete(act)
	local lastwhy = act.why
	Controller.complete(self,act)
	
	if act[1] == "h" then
		local slam = gm.ls:collide(self.pos.x, self.pos.y)[1]
		if not self.mute then gm.au.crash:setVolume(0) end
		if slam then
			gm.ls:collapse(slam.id, self.pos.x, self.pos.y)
			if slam.wkey then
				if not gm.ls.watermark[slam.wkey] or gm.ls.watermark[slam.wkey]<slam.at.y then
					gm.ls.watermark[slam.wkey] = slam.at.y
				end
				if _DEBUG then print({slam.at:str(), slam.pxat:str()}) end
				slam.wkey = nil
			end
			if slam.onClose then slam:onClose() end
		end
	end
	
	local stilldown = false
	for k,v in pairs(down) do
		if lastwhy and keymap[lastwhy] == k and lastwhy ~= "hammer" then stilldown = true end
	end

	if not stilldown then
		if self.storedwhy then
			lastwhy = self.storedwhy
			self.storedwhy = nil
		else
			lastwhy = nil
		end
	end

	if lastwhy then
		self.act = false -- Wrote myself into a corner.
		if lastwhy == "hammer" then
			self:hammer(lastwhy)
		else
			self:cursor(lastwhy)
		end
		return self.act
	end
end

function PlayerController:process(act)
	Controller.process(self, act)
	if act[1]=="h" and not self.mute then
		local progress = (act.step - 1) / (act.to - 1)
		gm.au.crash:setPitch(5+progress*5)
		gm.au.crash:setVolume(1-progress)
	end
end

-- Believed not used in this game.

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

-- Level container

class "LevelCanvas"

function LevelCanvas:LevelCanvas(spec)
	pull(self, spec)
end

function LevelCanvas:gfx()
	return self.g
end

class "LevelTile"

function LevelTile:LevelTile(spec)
	pull(self, spec)
end

function LevelTile:halt()
	return self.opq
end

class "LevelScreen"

local ls_id_generator = 1

km.original_umbrella = {[3]=2,[4]=2}

function LevelScreen:LevelScreen(spec)
	pull(self, {id=ls_id_generator, rainy={}, umbrella=tableCopy(km.original_umbrella), watermark = {}})
	pull(self, spec)
	if not self.g then self.g = gfxcontainer() end
	self.bg = {}
	self.tenant = SparseGrid()
	
	ls_id_generator = ls_id_generator + 1
end

function LevelScreen:pushbg()
	local g = SparseGrid()
	table.insert(self.bg, g)
	return #self.bg
end

function LevelScreen:loadNext(s, spec)
	local id = self:pushbg()
	s:load(self, id, nil, spec)
	return id
end

function LevelScreen:loadAt(id, s, spec)
	s:load(self, id, nil, spec)
end

-- Load self into levelscreen -- Why the back and forth? This sucks.
function LevelCanvas:load(ls, id, grid, spec)
	grid = grid or ls.bg[id]
	local g = self:gfx()
	local gw,gh = math.floor(g.w/km.cell.x), math.floor(g.h/km.cell.y)

	for x=1,gw do
		for y=1,gh do
			local at = P(x ,y - (gh-km.lh))
			local pxat = ls:rebase(at)
			local anyopq, allopq = bridge:gfxscan(g, pxat.x, pxat.y, km.cell.x, km.cell.y)
			
			if anyopq then
				self:loadOne(ls, grid, x, y, id, at, pxat, spec, allopq)
			end
		end
	end
end

function LevelCanvas:loadOne(ls, grid, x, y, id, at, pxat, spec, allopq)	
--	print({ls,grid,x,y,id,at:str(),pxat:str(),spec,allopq})
	local xoff = spec.xoff or 0
	local tile = LevelTile({
		at=at, pxat=pxat, s=self, id=id, opq=allopq, rainy=spec.rainy, immortal=spec.immortal
	})
	if spec.float then
		grid:set(x+xoff,y,tile)
	else
		ls:pushcol(grid, tile, x+xoff)
	end
end

function LevelScreen:pushcol(grid, tile, x)
	for y=1,10000 do
		local check = grid:get(x,y)
		if not check then
			grid:set(x,y,tile)
			break
		end
	end
end

-- Load entity into self
function LevelScreen:load(e)
	local t = self.tenant:get(e.pos.x, e.pos.y, {})
	t[e.id] = e
end

-- Unload entity from self
function LevelScreen:unload(e)
	local cell = self.tenant:get(e.pos.x,e.pos.y)
	if cell then cell[e.id] = nil end
end

-- Ignores tenants
function LevelScreen:collide(x,y)
	local result = {}
	for i,v in ipairs(self.bg) do
		local tile = v:get(x,y)
		if tile then
			table.insert(result, tile)
			if tile:halt() then break end
		end
	end
	return result
end

function LevelScreen:collapse(id,x,y)
	local layer = self.bg[id]
	local tlast = layer:get(x,y)
	local treset = tlast and tlast.immortal and tlast
	
	for y2 = y, 10000 do
		if not tlast then break end
		tlast = layer:get(x,y2+1)
		layer:set(x,y2,tlast)
	end
	
	if treset then
		self:pushcol(layer, treset, x)
	end
end

function LevelScreen:watermark_key_all(wkey,id,x,y)
	if not x then
		local any = false
		for x=1,10000 do
			if self:watermark_key_all(wkey,id,x,y) then
				any = true
			else
				break
			end
		end
		return any
	end
	if not y then
		local any = false
		for y=1,10000 do
			if self:watermark_key_all(wkey,id,x,y) then
				any = true
			else
				break
			end
		end
		return any
	end
	
	local layer = self.bg[id]	
	local tile = layer:get(x,y)
	if not tile then return false end
	tile.wkey = wkey
--	print({"THIS:",x,y,"IS",tile.at.x,tile.at.y,tile.wkey})
	return true
end

function LevelScreen:insert()
--	gm.ls[self.id] = self
--	gm.ls[self.channel] = self
	return self
end

function LevelScreen:blit(at, tile, opq)
	local g = tile.s:gfx()
	local func = opq and self.g.pxcopy or self.g.pxcopy_blend
	func(self.g, g, at.x, at.y, tile.pxat.x, tile.pxat.y, km.cell.x, km.cell.y)
	if tile.rainy and opq then table.insert(self.rainy, {at=at,pxat=tile.pxat}) end
end

function LevelScreen:reblank()
	self.g:pxclear()
	for x=1,km.lw do
		for y=1,km.lh do
			local at = self:rebase(P(x,y))
			local tiles = self:collide(x,y)
			local tilecount = #tiles
			if tilecount > 0 then
				self:blit(at, tiles[tilecount], true)
				for i=tilecount-1,1,-1 do
					self:blit(at, tiles[i], false)
				end
			end
		end
	end
end

function LevelScreen:flushrain(rain)
	for i,v in ipairs(self.rainy) do
		local umbrella = self.umbrella[v.at.x/32]
		if (not umbrella) or v.at.y/32 < umbrella then
			v.s = rain
			self:blit(v.at, v)
		end
	end
	self.rainy = {}
end

function LevelScreen:rebase(p)
	return P( (p.x-1)*km.cell.x, (km.lh-p.y)*km.cell.y )
end

-- Junk

function blittext(g, x, y, text, inv, xoff, yoff)
	inv = inv or false 
	xoff = xoff or 0
	yoff = yoff or 0
	if not gm.english then
		gm.english = gfxcontainer()
		gm.english:load_image("media/english.png")
	end
	g:pxcopy_text(gm.english, text, x*7+xoff, y*8+yoff, inv, -1, -1, 7, -1)
end

function blittext_centered(g, x, y, w, text, inv, xoff, yoff)
	if #text > w then text = text:sub(1,w) end
--	print({x,y,w,(w-#text)/2,math.floor(x+(w-#text)/2)})
	blittext(g, math.floor(x + (w - #text)/2), y, text, inv, xoff, yoff) 
end