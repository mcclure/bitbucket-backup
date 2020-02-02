-- Pregame on load

fm = fm or {}
gm = {onTick = {}, onInput = {}, onInputStack = Stack()}
km = {BLOCK=1, SPRITE=2, TRIGGER=3, ANCHOR=4, climbmax=1, text_timeout_on = 12, textscale = 1}

function pushInput() gm.onInputStack:push(gm.onInput) gm.onInput = {} end
function popInput() local top = gm.onInputStack:pop() if top then gm.onInput = top end end

-- Input support

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

-- Subdivide stuff

function dim3(x,y,z,default)
	local t = {}
	t.x = x t.y = y t.z = z
	for ix=1,x do
		local tx = {}
		t[ix] = tx
		for iy=1,y do
			local ty = {}
			tx[iy] = ty
			if (default ~= nil) then
				for iz = 1,z do
					ty[iz] = default
				end
			end
		end
	end
	return t
end

class "Blocker"

function Blocker:Blocker(spec)
	pull(self,{x=1,y=1,z=1})
	pull(self,spec)
	if not self.vox then self.vox = dim3(self.x,self.y,self.z) end
end

function Blocker:clone()
	local b = Blocker({x=self.x,y=self.y,z=self.z})
	for x=1,self.vox.x do for y=1,self.vox.y do for z = 1,self.vox.z do
		b.vox[x][y][z] = self.vox[x][y][z]
	end end end
	return b
end

function Blocker:subdivide()
	local b = Blocker({x=self.x*2,y=self.y*2,z=self.z})
	for x=1,self.vox.x do for y=1,self.vox.y do for z = 1,self.vox.z do
		local what = math.random()
		for x2 = 0,1 do for y2 = 0,1 do-- for z2 = 0,1 do
			local result = self.vox[x][y][z]
			if result then
				if what < 0.25 then --result = not result
				elseif what < 0.50 then if (x2 == 0 and y2 == 0) or (x2 == 1 and y2 == 1) then result = not result end
				elseif what < 0.75 then if (x2 == 0 and y2 == 1) or (x2 == 1 and y2 == 0) then result = not result end
				end
			end
			b.vox[x*2+x2-1][y*2+y2-1][1] = result
		end end --end
	end end end
	return b
end

function killScreen(e)
	if e then e:getParentEntity():removeChild(e) end
end

-- xo,yo,zo are relative to d, rooted TOPLEFT
function Blocker:add(_spec)
	_spec.xo = _spec.xo or _spec.o
	_spec.yo = _spec.yo or _spec.o
	_spec.zo = _spec.zo or _spec.o
	local spec = tableMerge({d=1.0,xo=0,yo=0,zo=0,g=1},_spec)
	
	self.e = SceneEntity()
	spec.e:addChild(self.e)
	
	for x=1,self.vox.x do for y=1,self.vox.y do for z = 1,self.vox.z do
		if self.vox[x][y][z] then
			local e = ScenePrimitive(ScenePrimitive.TYPE_BOX, spec.d, spec.d, spec.d)
			e:setPosition( (x-0.5)*spec.d+spec.xo, (y-0.5)*spec.d+spec.yo, (z-0.5)*spec.d+spec.zo )
			if fm.rainbow then e:setColor(math.random(),math.random(),math.random(),1)
			else e:setColor(spec.g,spec.g,spec.g,1) end
			self.e:addChild(e)
		end
	end end end
end

function Blocker:remove()
	if self.e then 
		killScreen(self.e)
		self.e = nil
	end
end