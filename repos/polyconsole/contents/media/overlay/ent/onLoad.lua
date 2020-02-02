-- Entity lib - on load

pull(gm, {onTick = {}, onInput = {}, onInputStack = Stack()})

function pushInput() gm.onInputStack:push(gm.onInput) gm.onInput = {} end
function popInput() local top = gm.onInputStack:pop() if top then gm.onInput = top end end

-- Ents

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

-- Routers -- for key input

keymap = keymap or {}

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

-- Ents with lifetimes and timers

class "TimedEnt" (Ent)

function TimedEnt:TimedEnt(spec)
	Ent.Ent(self, spec)
end

function TimedEnt:insert()
	self.born = ticks
	return Ent.insert(self)
end

function TimedEnt:ticks()
	return ticks - self.born
end

class "Clock" (TimedEnt)

function Clock:Clock(spec)
print(spec.schedule)
	spec = tableMerge({schedule={},
	}, spec)
	TimedEnt.TimedEnt(self, spec)
end

function Clock:onTick()
	local t = self:ticks()
	local f = self.schedule[t]
	if f then
		self.schedule[t] = nil
		f(self)
	end
	if not tableTrue(self.schedule) then self:die() end -- Schedule exhausted
end

class "Anim" (TimedEnt)

-- Spec options: "length", "target", "call" [OR] "field", "to" w/optional "from" [OR] "value" function
function Anim:Anim(spec)
	TimedEnt.TimedEnt(self, spec)
	if self.field and self.target and not self.from then self.from = self.target[self.field] end
end

function Anim:onTick()
	local value = self:value()
	self:set(value)

	if self.length and self:ticks() >= self.length then
		self:finish()
		self:die()
	end
end

function Anim:value()
	local ticks = self:ticks()
	local progress = ticks / self.length
	return self.from + progress*(self.to-self.from)
end

function Anim:set(value)
	if self.target then
		if self.call then
			self.call(self.target, value)
		else
			self.target[self.field] = value
		end
	end
end

function Anim:finish()
	if self.final then
		self:set(self.final)
	end
end