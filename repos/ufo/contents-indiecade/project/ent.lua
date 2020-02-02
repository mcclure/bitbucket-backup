-- Autonomous entity lib

function pushInput() gm.onInputStack:push(gm.onInput) gm.onInput = {} end
function popInput() local top = gm.onInputStack:pop() if top then gm.onInput = top end end

-- Utilities

function pull(dst, src)
	if dst and src then
		for k,v in pairs(src) do
			dst[k] = v
		end
	end
end

function tableTrue(e) -- True if table nonempty
	return next(e) ~= nil
end

class.Queue() -- Queue w/operations push, pop, peek
function Queue:_init()
	self.low = 1 self.count = 0
end
function Queue:push(x)
	self[self.low + self.count] = x
	self.count = self.count + 1
end
function Queue:pop()
	if self.count == 0 then
		return nil
	end
	local move = self[self.low]
	self[self.low] = nil
	self.count = self.count - 1
	self.low = self.low + 1
	return move
end
function Queue:peek()
	if self.count == 0 then
		return nil
	end
	return self[self.low]
end
function Queue:empty()
	return self.count == 0
end
function Queue:at(i)
	return self[self.low + i - 1]
end
function Queue:ipairs()
  local function ipairs_it(t, i)
    i = i+1
    local v = t:at(i)
    if v ~= nil then
      return i,v
    else
      return nil
    end
  end
  return ipairs_it, self, 0
end

class.Stack() -- Stack w/operations push, pop, peek
function Stack:_init()
	self.count = 0
end
function Stack:push(x)
	self.count = self.count + 1
	self[self.count] = x
end
function Stack:pop()
	if self.count == 0 then
		return nil
	end
	local move = self[self.count]
	self[self.count] = nil
	self.count = self.count - 1
	return move
end
function Stack:peek()
	if self.count == 0 then
		return nil
	end
	return self[self.count]
end
function Stack:empty()
	return self.count == 0
end

-- Entity utilities

km = {prio={count=5, default=3}}
gm = {}
doom = {}
pull(gm, {onTick = {}, onInput = {}, onInputStack = Stack()})

for i=1,km.prio.count do table.insert(gm.onTick, {}) end

function entity_tick()
	for k,v in pairs(gm.onInput) do
		v:onInput()
	end

	for k1,v1 in pairs(gm.onTick) do
		for k2,v2 in pairs(v1) do
			v2:onTick()
		end
	end
	
	for i,v in ipairs(doom) do v() end
	doom = {}
end

-- Ents

local ent_id_generator = 1

class.Ent()
function Ent:_init(spec)
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

function Ent:insert(prio)
	self.prio = prio or km.prio.default
	if self.onInput then gm.onInput[self.id] = self end
	if self.onTick then gm.onTick[self.prio][self.id] = self end
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

function Ent:die()
	if self.dead then return end -- Don't die twice
	self.dead  = true
	
	if self.onInput then
		gm.onInput[self.id] = nil
		for i=1,gm.onInputStack.count do
			gm.onInputStack[i][self.id] = nil -- Every part of you must die
		end
	end
	if self.onTick then gm.onTick[self.prio][self.id] = nil end
	gm.r:unregister(self)
	
	if self.die_with then
		for i,v in ipairs(self.die_with) do
			v:die()
		end
	end
	
	return self
end

function Ent:setListening(v) -- Only relevant to "autonomous" ents.
	self.listener = v and Queue() or nil
	listeners[self.id] = self.listener
end

-- Routers -- for key input

-- keymap = keymap or {}

class.Router(Ent)

function Router:_init(spec)
	self:super(spec)
	if not self.route then
		self.route = { pressed={}, down={} }
	end
end

function Router:register(kind, action, obj, func)	
	local key = action -- keymap[action]
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
		if engine.ui_state.pressed[key] then self:execute(keyt) end
	end
	for key, keyt in pairs(self.route.down) do
		if engine.ui_state.down[key] then self:execute(keyt) end	
	end
end

-- Ents with lifetimes and timers

class.TimedEnt(Ent)

function TimedEnt:_init(spec)
	self:super(spec)
end

function TimedEnt:insert()
	self.born = engine.ticks
	return Ent.insert(self)
end

function TimedEnt:ticks()
	return engine.ticks - self.born
end

class.Clock(TimedEnt)

function Clock:_init(spec)
	spec = tableMerge({schedule={},
	}, spec)
	self:super(spec)
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

class.Anim(TimedEnt)

-- Spec options: "length", "call" [OR] "target" + "field", "to" w/optional "from" [OR] "value" function
function Anim:_init(spec)
	self:super(spec)
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
	local target = self.target or self
	if self.call then
		self.call(target, value)
	else
		target[self.field] = value
	end
end

function Anim:finish()
	if self.final then
		self:set(self.final)
	end
end