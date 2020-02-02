-- Pregame -- On Load

pull(km, {maxicons=7})

km.blankColor = Color(0,0,0,0)
km.secretColor = Color(0,0,0,255)
km.colors = { Color(255,0,0,255), Color(0,255,0,255), Color(0,0,255,255) }

-- Image utility

function textureLoad(name, clamp)
	local ci = clamp and 1 or 0
	if not fm.texture_cache then fm.texture_cache = {} end
	if not fm.texture_cache[ci] then fm.texture_cache[ci] = {} end
	if not fm.texture_cache[ci][name] then
		local image = a(Image(name))
		fm.texture_cache[ci][name] = Services.MaterialManager:createTextureFromImage(image, clamp, false)
	end
	return fm.texture_cache[ci][name]
end

if _DEBUG then
	function texture_reset(nobirth)
	for k,v in pairs(fm.texture_cache) do if v.__delete then delete(v) end end
	fm.texture_cache = nil if not nobirth then bridge:rebirth() end end
end

function insertImage(e, i)
	if not e then return nil end
	local einfo = {e:getPosition(), e:getWidth(), e:getHeight()}
	local ne = ScreenImage.ScreenImageWithTexture(textureLoad(i, true))
	vSetPosition(ne, einfo[1])
	ne:setPositionMode(ScreenShape.POSITION_CENTER)
--	ne:setScale(einfo[2]/ne:getImageWidth(), einfo[3]/ne:getImageHeight())
	screen():addChild(ne)
	screen():removeChild(e)
	return(ne)
end

function insertBlank(e)
	local einfo = {e:getPosition(), e:getWidth(), e:getHeight()}
	local ne = ScreenShape(ScreenShape.SHAPE_RECT, einfo[2], einfo[3])
	vSetPosition(ne, einfo[1])
	ne:setPositionMode(ScreenShape.POSITION_CENTER)
	ne:setColor(0,0,0,0)
	screen():addChild(ne)
	screen():removeChild(e)
	return(ne)
end

function sprite(x, y)
	return string.format("media/sprite/%s-%s.png", x or "nil", y or "nil")
end

function spriteLoad(prefix, list, clamp)
	for i,v in ipairs(list) do
		textureLoad(sprite(prefix, v), clamp)
	end
end

-- Game logic

function beats(a,b) -- Using modulo math, create a <=> for "win if x = x + 1" in 1..3
	if not (a or b) then return 0 end
	if not a then return 1 end
	if not b then return -1 end
	
	local d = b - a
	d = (d + 4)%3
	d = d - 1
	return d
end

-- Actual game

class "Player" (Ent)

function Player:Player(spec)
	local controls = {}
	local function j1(a,b) return string.format("%s_%s",a,b) end
	local function j(a,b,c) if b then a=j1(a,b) end if c then a = j1(a,c) end return a end
	local function n(x) return j(spec.is, x) end
	for i=1,3 do controls[n(i)] = self.move end
	controls[n("cancel")] = self.cancel
	spec = tableMerge({pressed=controls, model={}, view={},
	}, spec)
	Ent.Ent(self, spec)

	-- Reset model
	pull(self.model, {moves=Queue()})
	
	-- Setup view
	local function v(a,b) return ScreenEntity(id(j(a,b,spec.is))) end
	local icons = {} for i=1,km.maxicons do table.insert(icons, v("move",i)) end
	pull(self.view, {body=v("player"), barback=v("barback"), icons=icons})
	pull(self.view, {barwidth = self.view.barback:getWidth(), barheight = self.view.barback:getHeight(), barbase = self.view.barback:getPosition()})
	local inset = self.view.barheight*0.2
	self.view.barwidth = self.view.barwidth - inset
	self.view.barheight = self.view.barheight - inset

	self:adjustBody(nil)

	self.view.barin = ScreenShape(ScreenShape.SHAPE_RECT, self.view.barwidth, self.view.barheight)
	self.view.barin:setPosition(self.view.barbase.x, self.view.barbase.y)
	self.view.barin:setColor(255,255,255,255)
	screen():addChild(self.view.barin)
	
	-- Setup audio
	self.view.notes = {}
	if self.notebase then
		for i,v in ipairs({0,4,7}) do
			table.insert(self.view.notes, r(newTone(self.notebase+v)))
		end
		self.view.notes.land = r(newBeep(self.notebase+24))
		self.view.notes.deny = r(newBeep(self.notebase-5,2))
		
		self.view.notes.cancel = r(newSquare(self.notebase))
		
		self.view.notes.ouch = r(Sound("media/sound/ouch.ogg"))
		self.view.notes.ouch:setPitch(pitchFrom(-7))
		self.view.notes.ouch:setVolume(0.05)
		
		self.view.notes.fail = r(newFall(self.notebase))
	end

	self:moveRedraw()
end

function Player:adjustBody(which)
	self.view.body = insertImage( self.view.body, sprite(self.base, which) )
end

function Player:move(n)
	if self.ref:frozen() then return end
	if self.model.moves.count >= self.ref:queueSize() then
		self:deny()
		return
	end

	n = tonumber(string.match(n, "^[^_]+_([^_]+)$"))
	
	-- Model
	self.model.moves:push(n)
	
	-- View
	local sound = self.view.notes[n] if sound then sound:Play(false) end
	self:moveRedraw()
	
	-- Upstream
	self.ref:onMove(self)
end

function Player:cancel()
	if self.ref:frozen() then
		return
	end
	
	-- Model
	self.model.moves = Queue()
	
	-- View
	self:moveRedraw()
	self.view.notes.cancel:Play(false)
	
	-- Upstream
	self.ref:onCancel()
end

function Player:moveDebug()
	local s = string.format("Player %s:\n", self.is)
	for i,v in self.model.moves:ipairs() do
		s = s .. string.format("\t%s:%s\n",i,v)
	end
	print(s)
end

function Player:moveRedraw()
--	self:moveDebug()
	
	for i=1,km.maxicons do
		local icon = self.view.icons[i]
		local state = self.model.moves:at(i)
		if state then
			if self.ref and self.ref.state == "prepare" then
				icon = insertImage(icon, sprite("icon", nil))
			else
				icon = insertImage(icon, sprite("icon", state))
			end
		else
			icon = insertBlank(icon)
		end
		
		self.view.icons[i] = icon
	end
end

function Player:healthRedraw()
	local remaining = clamp(0, self.model.health/self.ref.rules.maxhealth)
	if self.view.barhealth then screen():removeChild(a(self.view.barhealth)) end
	local width = remaining*self.view.barwidth
	self.view.barhealth = ScreenShape(ScreenShape.SHAPE_RECT, width, self.view.barheight)
	self.view.barhealth:setPosition(self.view.barbase.x + (self.view.barwidth-width)/2 * (self.is=="a" and -1 or 1), self.view.barbase.y)
	self.view.barhealth:setColor(1,0,0,1)
	screen():addChild(self.view.barhealth)
end

function Player:begin()
	if not self.model.health then self.model.health = self.ref.rules.maxhealth end
	self:healthRedraw()
end

function Player:clearMoves()
	self.model.moves = Queue()
	self:moveRedraw()
end

function Player:ouch()
	self.view.notes.ouch:Play(false)
end

function Player:fail()
	self.view.notes.fail:Play(false)
end

function Player:deny()
	if not self.view.notes.deny:isPlaying() then 
		self.view.notes.deny:Play(false)
	end
end

function Player:land()
	self.view.notes.land:Play(false)
end

-- RULES

function saveRules(t, filename)
	bridge:saveTableIntoFile(filename, "rules", t)
end

function loadRules()
	local external = "rules.xml"
	local r = nil
	if not _DEBUG then bridge:loadTableFromFile(external, true) end
	if not r then
		r = bridge:loadTableFromFile("media/rules.xml")
		if r then
			saveRules(r, external)
		end
	end
	return r
end

class "Ref" (TimedEnt)

-- States: prepare, fight, tryagain, sleep
function Ref:Ref(spec)
	spec = tableMerge({state="prepare", combo=1,
		view={message=ScreenLabel(ScreenShape(ScreenMesh((ScreenEntity(id("message")))))),
		restart=id("restart")},
	}, spec)
	TimedEnt.TimedEnt(self, spec)
	self.view.restart.visible = false
	if not self.rules then self.rules = loadRules() end	
end

function Ref:insert()
	self.a:begin()
	self.b:begin()
	return TimedEnt.insert(self)
end

function Ref:onTick()
	local message = self.view.message
	if self.state == "prepare" then
		local text = nil
		local base = math.floor(FPS/2)
		local t = self:ticks()
		if t == base       then text = "3..." end
		if t == base*2   then text = "2..." end
		if t == base*3 then text = "1..." end
		if t == base*4 then
			text = "FIGHT!"
			Clock({schedule={[base]=function(self) message:setText("") self:die() end}}):insert()
			self.state = "fight"
			self:moveRedraw()
			if self.movebase then self.movebase = t end
		end
		if text then
			message:setText(text)
		end
	end
	if self.state == "fight" then
		local moveage = self:moveage()
		if moveage then
			if moveage >= self:delay() then
				local ma = self.a.model.moves:pop()
				local mb = self.b.model.moves:pop()
				
				self.a:adjustBody(ma)
				self.b:adjustBody(mb)
				
				if ma or mb then
					local beat = beats(ma,mb)
					if beat == 0 then -- Parry
						self.leader:fail() -- Break combo
						self.lastwin = nil
						self.combo = 1 -- Remember: Alters behavior of delay()
						
						for i,player in ipairs({self.a, self.b}) do
							local position = player.view.body.position
							Anim({target=position, length=self:delay()-2, field="x", shake=math.min(self:delay()-1,8),
								value=function(self) return self.from + self.shake*((i%2)*-2+1) end,
							final=position.x}):insert()
						end
					else -- Attack
						local winner = beat == -1 and self.a or self.b
						local loser  = beat == -1 and self.b or self.a
						
						loser.model.health = loser.model.health - self.rules.hit[1]
						loser:healthRedraw()
						winner:land()
						
						if not self.lastwin or self.lastwin == winner.is then -- Remember: Alters behavior of delay()
							self.combo = math.min(self.combo + 1, #self.rules.delay)
						else
							self.combo = 1
						end
						
						self.lastwin = winner.is
						self.leader = winner
						
						if loser.model.health >= 0 then -- Survived
							local position = loser.view.body.position
							Anim({target=position, length=6, field="x", shake=math.min(self:delay()-1,5),
								value=function(self) return self.from + self.shake*((self:ticks()%2)*2-1) end,
							final=position.x}):insert()
						else -- Died
							local body = loser.view.body
							local w, h = body:getWidth(), body:getHeight()
							local adjust = (w - h)/2
							body:setRotation(-90*beat)
							body.position.y = body.position.y - adjust
							body.position.x = body.position.x + adjust*beat
						
							loser:ouch()
							
							self:call(winner)
						end
					end
					
					local sound = au[ beat == 1 and mb or ma ]
					sound:setVolume(beat == 0 and 0.5 or 1)
					sound:Play(false)
					
					self.movebase = self:ticks()
				end
				
				if not (self.a.model.moves.count > 0 or self.b.model.moves.count > 0) then
					self.movebase = nil
					self.lastwin = nil
					self.combo = 1
				end
				
				self:moveRedraw()
			end
		end
	end
	
	if self.state == "tryagain" and pressed[KEY_SPACE] then
		gm.need_restart = true
	end
end

function Ref:connect(a,b)
	self.a = a
	a.ref = self
	self.b = b
	b.ref = self
	return self
end

function Ref:moveRedraw()
	self.a:moveRedraw() self.b:moveRedraw()
end

function Ref:queueSize()
	if self.state == "prepare" and self.rules.minstart and self.rules.minstart>0 then
		return self.rules.minstart
	elseif self:frozen() then
		return 0
	else
		return self.rules.queuesize
	end
end

function Ref:moveage()
	return self.movebase and self:ticks()-self.movebase
end

function Ref:onMove(mover)
	if not self.leader then self.leader = mover end
	if not self.movebase then
		self.movebase = self:ticks()
	end
end

function Ref:onCancel()
	if not (self.a.model.moves.count > 0 or self.b.model.moves.count > 0) then
		self.movebase = nil
	end
end

function Ref:delay()
	return self.rules.delay[self.combo]
end

function Ref:frozen()
	return self.state == "sleep" or self.state == "tryagain"
end

function Ref:call(winner)
	self.view.message:setText(string.format("Player %d wins", winner.is=="a" and 1 or 2))
	self.a:clearMoves() self.b:clearMoves()
	self.state = "tryagain"
	self.view.restart.visible = true
end