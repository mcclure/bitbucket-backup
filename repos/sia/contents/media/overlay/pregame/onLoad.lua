-- Pregame on load

screenNames = {}
roomObj = ""

screens = {}
shaders = {}
act = Queue()
gm = {halted=false}
km = {wait=2, handticks = 20, tickon = 80/60, rayon = 7/60, dclickwait=10, dblinkrate=10, immortaldos = false, helmetfactor=10, nohelmetfactor=1}
if not fm then fm = {} end

function plant(child)
	if child.sensor then return end
	bridge:setVisible(child, false)
	act:push({"enable", target=child, wait=km.wait})
end

function removeScreen(screenName, target)
	if not target then target = screens[screenName] end
	if not target then return end
	for i=1,target:getNumChildren() do
		local child = target:getChild(target:getNumChildren()-i)
		bridge:room_remove_screen(child, false)
	end
	screens[screenName] = nil
	delete(target)
end

function actRemoveScreen(screenName, target)
	act:push({"do", this=function() removeScreen(screenName, target) end})
end

function actInstantAddScreen(name, objname, dontPlant)
	act:push({"do", this=function() addScreen(name, objname, dontPlant) end})
end

function actExpedite()
	act:push({"do", this=function() gm.expedite = true end})
end

function trashScreen(screenName, target)
	if not target then target = screens[screenName] end
	for i=1,target:getNumChildren() do
		local child = target:getChild(target:getNumChildren()-i)
		if not child.sensor then
			act:push({"disable", target=child, wait=km.wait})
		end
	end
	actRemoveScreen(screenName, target)
end

function addScreen(name, objname, dontPlant, filename)
	local fullname = string.format("media/maps/%s.svg", filename or name)
	if objname and #objname then objname = string.format("media/levels/%s", objname) end
	local newscreen = bridge:standalone_screen(fullname, objname)

	if _DEBUG and screens[name] then print("WARNING: DUPE SCREEN: " .. name) end
	screens[name] = newscreen

	newscreen:setScreenShader("FilterMaterial")

	shaders[name] = shaderBindings(newscreen,
		{radius=(fm.helmet and km.helmetfactor or km.nohelmetfactor)/surface_width/2, aspect=(surface_width/surface_height)})
	
	if not dontPlant then
		plantScreen(newscreen)
	end
end

function plantScreen(newscreen)
	for i=1,newscreen:getNumChildren() do
		local child = newscreen:getChild(i-1)
		
		-- Initially invisible entities stay invisible forever.
		if not bridge:getVisible(child) or string_empty(bridge:room_name(child)) then
			child.sensor = true
		else
			plant(child)
		end
	end
end

function go(name)
	if gm.halted then return end
	gm.halted = true
	for k,onescreen in pairs(screens) do
		trashScreen(k,onescreen)
	end

	act:push({"go", to=name, wait=km.wait,})
end

function killdos()
	if dos then
		if gltm then
			gltm.glittering = nil
			gltm.glitterat = nil
			gltm.wasat = nil
		end
		if immortaldos then
			dos:clear()
		else
			dos:die()
			dos = nil
		end
	end
end

class "Dialogue"
function Dialogue:Dialogue(spec)
	pull(self,{up=false, start="start"})
	pull(self,spec)
end
function Dialogue:invoke()
	if self.up then return end
	self.up = true
	
	for k,v in pairs(self.blank or {}) do
		local e = id(v)
		if e then
			act:push({"disable", target=e, wait=km.wait})
		end
	end
	if self.create then
		addScreen(self.create, km.roomObj)
	end
	addScreen("dialog",km.roomObj)
	act:push({"do", this=function()
		killdos()
		if not dos then
			dos = type_automaton()
			dos:insert()
		end
		glitter_update(dos, self.gstart)
		
		local dinner = id("dinner")
		local dinner_hit = a(ScreenEntity(dinner):getHitbox())
		local dinner_pos = a(dinner:getPosition())
		local lobound = dinner_pos.y + dinner_hit.y
		local hibound = lobound + dinner_hit.h
		local factor = dos:factor()
		local celly = factor * 8
		local dospad = (hibound-lobound - celly*6)/2
		local dosfloor = surface_height - (surface_height - celly*24)/2
		local offset_needed = dosfloor - (hibound - dospad)

		self.celly = celly
		self.cellx = factor*7
		self.dosfrom = lobound+dospad
		self.dosfromx = (surface_width - self.cellx*40)/2
		self.offset_needed = offset_needed
		dos:apply_offset(0,-offset_needed)
	
		gm.dialog = self
		self:clear()
		self:branch(self.start)
	end})
end
function Dialogue:branch(to)
	self.at = self.tree[to]
	self.step = 1
	self:speak()
end
function Dialogue:speak()
	self.wholeclick = false
	self.say = ""
	self:feed(self.at.say[self.step])
	self.options = {}
	if self.step == #self.at.say and
		self.at.options and #self.at.options then
		for i,opt in ipairs(self.at.options) do
			self:feed("\n\n  ")
			act:push({"do", this=function()
				table.insert(self.options, {x=self.pm.x, y=self.pm.y, len=#opt.say, go=opt.go, say=opt.say})
			end})
			self:feed(opt.say)
		end
	else
		act:push({"do", this=function()
			self.wholeclick = ticks
		end})
	end
end
function Dialogue:feed(say)
	for c in say:gmatch(".") do
		act:push({"di", a=c, wait=(c=="\n" and 0 or 1)})
	end
end
function Dialogue:tick()
	if self.wholeclick then
		dos:set(39,23,math.floor((self.wholeclick-ticks+1)/km.dblinkrate)%2==0 and ">" or " ")
	end
end
function Dialogue:click(x,y,down)
	if self.clickblock then return end
	
	if self.wholeclick then
		if self.step == #self.at.say then
			if self.at.onDismiss then
				self.at:onDismiss(self)
			else
				self:teardown()
			end
		else
			self.step = self.step + 1
			self:clear()
			self:speak()
		end
		return
	end
	
	local xcell, ycell = 
		math.floor((x-self.dosfromx)/self.cellx),
		math.floor((y-self.dosfrom)/self.celly)
	
	for i,opt in ipairs(self.options) do
		if opt.x <= xcell and opt.x + opt.len > xcell and opt.y == ycell+18 then
			self.clickblock = true
			dos:set(opt.x,opt.y,opt.say,true)
			act:push({"do", wait=km.dclickwait, this=function() 
				--dos:set(opt.x,opt.y,opt.say)
				if opt.onDismiss then
					opt:onDismiss(self)
				else
					self:clear()
					self:branch(opt.go)
				end
			end})
			break
		end
	end
end
function Dialogue:clear()
	dos:clear()
	self.clickblock = false
	self.pm = {x = 0, y = 18}
end

function Dialogue:add(say)
	for c in say:gmatch(".") do
		if c == "\n" then
			self.pm.x = 0 self.pm.y = self.pm.y + 1
		else
			dos:set(self.pm.x,self.pm.y,c)
			self.pm.x = self.pm.x + 1
			if self.pm.x > 39 then self.pm.x = 0 self.pm.y = self.pm.y + 1 end
		end
	end
end
function Dialogue:teardown()
	self.clickblock = false self.wholeclick = false
	
	killdos()
	trashScreen("dialog",km.roomObj)
	if self.create then
		trashScreen(self.create, km.roomObj)
	end
	for k,v in pairs(self.blank or {}) do
		local e = id(v)
		if e then
			act:push({"enable", target=e, wait=km.wait})
		end
	end
	act:push({"do", this=function() self.up = false gm.dialog = nil end})
end

class "MiniTimer"

function MiniTimer:MiniTimer(cycle,modes)
	self.cycle = cycle
	self.modes = modes
	self.start = Services.Core:getTicksFloat()
end

function MiniTimer:get()
	local now = Services.Core:getTicksFloat()
	local since = now - self.start
	if self.modes then
		local count = math.floor(since/self.cycle)
		if self.modes > 0 then count = count % self.modes end
		return count
	else
		if since > self.cycle then
			self.start = now - ( now % self.cycle )
			return true
		else
			return false
		end
	end
end

if not fm.tickon then fm.tickon = MiniTimer(km.tickon) end