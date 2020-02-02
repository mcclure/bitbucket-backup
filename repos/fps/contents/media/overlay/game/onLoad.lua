-- Game logic load

-- Set up "spotlight" effect
--screen():setScreenShader("FilterMaterial")
--shader = shaderBindings(screen(), 
--	{center=Vector3(0.1,0.1,0.0), radius=0.1, intensity=100.0, aspect=(surface_width/surface_height)})

Services.Core:enableMouse(false)

-- Sound

m_moving:setVolume(0) 
m_moving:setPitch(1)
m_ambient:setPitch(1)
m_panic:setVolume(0.75)
m_panic:setPitch(0.25)

s_uhit:setVolume(0.75)
s_umiss:setVolume(0.75)
s_ehit:setVolume(2)

s_land:setVolume(0.5)

-- Letter texture dispenser

allLetters = r(Image("media/english.png"))
letterCache = {}
function texForLetter(c)
	if letterCache[c] then return letterCache[c] end
	local asc = string.byte(c)
	local xt = asc % 16 local yt = 15-math.floor(asc / 16)
	local i = r(bridge:textureFromImage(a(bridge:subImage(allLetters, xt*8, 64+yt*8, 7, 9))))
	letterCache[c] = i
	return i
end

-- Machines

if not gm then -- Game state: persists across rebirths
	gm = { at_level = 1, player_hp = 4, }
	levelStack = {}
end

-- Level ("camera") state
cm = { angle = 0, mouseLooking = 1, drop = false, camera_height_mod = 0, music_mod = 1, deathPitch = 0, moving = false }
-- Konstants
km = { strafe = 4, estrafe=2, bubble = 2, turn = -1, winunder = 1, dismisslevelmsgat = 1.05, enemy_flash_for = 20,
	max_bottomlines = 3, scroll_bottomat = 60, scroll_topat = 5, sky = true, shotw = 0.1, shotdown = 0.1, shotlife = 5,
	outrolen = 30, los_ramp = 0.1, prox_bubble_inner = 5, prox_bubble_outer = 20, max_hp = 9, health_bar=true, 
	on_player_hit=1, }

enemies = {} -- There's an existing ptr cache so this may be redundant... or maybe not. Let's take no risks
amulet = {}
enemyqueue = Queue()
enemyqueue_next = nil
enemycount = 0
if not mouse_mod then mouse_mod = 1 end

-- Machine tools

function enemy(e) return e and enemies[e.__ptr] end
function noRotation(e)
	bridge:setAngularFactor(s:getPhysicsEntityBySceneEntity(e), 0)
end
function noMovement(e)
	bridge:setLinearFactor(s:getPhysicsEntityBySceneEntity(e), 0)
	noRotation(e)
end

function getPosition(e)
	return a(a(e:getTransformMatrix()):getPosition())
end

function vArgXz(v) -- Get vector angle on XZ axis
	return math.atan2(v.x, v.z);
end	
function vArgXy(v) -- Get vector angle on XY axis
	return math.atan2(v.x, v.y);
end

function stopWorld() -- Call on time to fade to black
	cm.drop = true
	cm.outro = 0
	cm.outro_dir = 1
	local outro_layer = r(Screen())
	cm.outro_fader = ScreenShape(SHAPE_RECT, surface_width, surface_height)
	cm.outro_fader:setColor(0.0,0.0,0.0,0.0)
	cm.outro_fader:setPosition(surface_width/2,surface_height/2)
	outro_layer:addChild(cm.outro_fader)
end

function updateEnemyFields(e)
	local them = getPosition(e)
	local you = playerPos
	local coll = a(bridge:getFirstEntityInRay(s, them, you))
	coll = coll and coll:entity()
	e.has_los = coll and coll.__ptr == player.__ptr

	local gap = vSub(you, them)
	e.distance = vNorm(gap)

	if e.has_los and e.distance > km.bubble then
		local dir = vNormalize(gap, km.estrafe)
		s:setVelocity(e, dir)
		bridge:activate(s:getPhysicsEntityBySceneEntity(e))
	end
end

function enemyUpdate() -- Call on each turn, manages enemy movement
	control_moving.nearest_prox = 10000 control_panic.nearest_prox = 10000
	control_moving.any_los = false control_panic.any_los = false
	local you = playerPos
	local updateFieldsAll = enemycount <= 10
	
--	print({enemyqueue and enemyqueue.count, enemyqueue_next and enemyqueue_next.count})
	if not updateFieldsAll then -- There are more enemies than we can plasibly update. Just update a few.
		for i = 1,5 do
			local e = enemyqueue:peek() -- Pop item from queue
			if not e then -- No items-- swap in other queue and try again
				if enemyqueue_next then
					enemyqueue = enemyqueue_next
					enemyqueue_next = nil
					e = enemyqueue:peek()
				end
			end
			if not e then break end -- Just plain no enemies.
			enemyqueue:pop() -- We actually captured an item, so pop it out of the queue
			if not e.doa then
				local oldLastAct = e.lastAct
				e.lastAct = ticks
				if not enemyqueue_next then enemyqueue_next = Queue() end
				enemyqueue_next:push(e)
				if oldLastAct == ticks then break end
				updateEnemyFields(e)
			end
		end
	end
	for p,e in pairs(enemies) do
		if updateFieldsAll then updateEnemyFields(e) end

		local boss = e.proto.boss
		if e.distance and e.distance < control_moving.nearest_prox then control_moving.nearest_prox = e.distance end
		if boss and e.distance and e.distance < control_panic.nearest_prox then control_panic.nearest_prox = e.distance end
		
		if e.flashAt then
			local progress = 1-(e.flashAt/km.enemy_flash_for)
			e:setColor( e.r*progress, e.g*progress, e.b*progress, 1 )
			if e.flashAt <= 0 then
				e.flashAt = nil
			else
				e.flashAt = e.flashAt - 1
			end
		end
			
		if e.has_los then
			control_moving.any_los = true
			if boss then control_panic.any_los = true end
			
			e.emote_wait = e.emote_wait - 1
			e.fire_wait = e.fire_wait - 1
			if e.fire_wait < 0 then
				s_ehit:Play()
				newShot(getPosition(e), you, false)
				newAlert(hurtMsg(e.proto))
				fireplan(e)
				gm.player_hp = gm.player_hp - km.on_player_hit
				if gm.player_hp == 1 then
					newAlert("You feel nearly dead.")
				elseif gm.player_hp == 0 then
					newAlert("Your senses leave you.")
					gm.killed_by = e.proto
					stopWorld()
				end
			end
			if e.emote_wait < 0 then
				if e.proto.emote and #e.proto.emote>0 then
					newAlert(emoteMsg(e.proto))
				end
				emoteplan(e)
			end
		end
	end
	
	for i,e in ipairs(amulet) do
		local them = getPosition(e)
		them.y = you.y
		local dist = vNorm(vSub(you,them))
		if dist < 1.0 then
			gm.won_whole_game = true
			cm.just_beat_boss = true
			km.max_bottomlines = 24
			stopWorld()
		end
		local shader = math.cos( (ticks-e.droppedAt)/20*math.pi )/2 + 0.5
		e:setColor(shader,shader,shader,1)
	end
end

function eliminate(e) -- Call to delete an enemy
	if not e then return nil end
	enemies[e.__ptr] = nil
	enemycount = enemycount - 1
	e.doa = true
	Polycore.__ptr_lookup[ s:getPhysicsEntityBySceneEntity(e).__ptr ] = nil -- https://github.com/ivansafrin/Polycode/issues/106
	s:removePhysicsChild(e)
	delete(e)
end

function fireplan(e) -- Set an enemy's next fire time
	e.fire_wait = math.random(e.proto.firetimelow,e.proto.firetimehigh)
end
function emoteplan(e) -- Set an enemy's next emote time
	e.emote_wait = math.random(e.proto.emotetimelow,e.proto.emotetimehigh)
end

-- Build monster tables

local monstersByLetter = {}
class "Monster"

function Monster:Monster(spec)
	pull(self, {firetimelow = 5,firetimehigh=60*5, emotetimelow=60*2, emotetimehigh=60*10, hp = 2})
	if spec.i then pull(self,monstersByLetter[spec.i]) end
	pull(self,spec)
end

monsters = {}
for i,m in ipairs(monster_spec) do
	if m[1] then monstersByLetter[m[1]] = m end
end
for i,m in ipairs(monster_spec) do
	table.insert(monsters, Monster(m))
end

-- Build map (surroundings)

bg = r(Scene()) -- "Horizon" layer

s = r(PhysicsScene()) -- Objects layer

alpha = r(Scene()) -- Shots layer

local ground = ScenePrimitive(TYPE_PLANE, 100, 100)
ground:setColor(0,0,0,0)
ground:setMaterialByName("GroundMaterial")
s:addPhysicsChild(ground, SHAPE_PLANE, 0.0)

-- DOS

dos = type_automaton()
dos:insert()
dos:set_centered(0,11,40,string.format("MARS LEVEL %d", gm.at_level))
levelmsg = true
topscroll_start = nil topscroll_msg = nil topscroll_blackout = false
bottomscroll_start = nil bottomscroll_queue = Queue() need_alert_cleanup = false

function dosclear(y) -- Erase one line of DOS screen
	dos:set_centered(0,y,40,"")
end

function newSignal(_str) -- Insert a string into top line horizontal scroll
	local str = _str
	topscroll_blackout = str:sub(#str) == "~"
	if topscroll_blackout then str = str:sub(1,#str-1) end
	topscroll_msg = str
	topscroll_start = ticks
end

function newAlert(str) -- Insert a string into bottom vertical scroll
	bottomscroll_queue:push(str)
	bottomscroll_start = ticks
	while bottomscroll_queue.count > km.max_bottomlines do bottomscroll_queue:pop() end
end

-- Handle gunfire

shots = Queue()
if gm.won_whole_game then -- Hypershots are shots which regenerate each time they hit
	hypershots = Queue() hypershots_next = Queue()
else
	hypershots = nil hypershots_next = nil
end

function newShot(from, to, is_player, is_initial) -- Create the *animation* of a shot
	local across = vSub(to,from)
	local center = vAdd(from, vMult(across, 0.5)) center.y = from.y - km.shotdown
	local len = vNorm(across)
	local ang = vArgXz(across)
	local shot = ScenePrimitive(TYPE_PLANE, km.shotw, len)
	if is_player then
		if (not is_initial) and math.random(2) == 1 then
			shot:setColor(1.0,1.0,1.0,0.5)
		else
			shot:setColor(1.0,0.5,0.5,0.5)
		end
	else
		shot:setColor(1,1,0,0.5)
	end
	vSetPosition(shot, center)
	shot:setYaw(ang / math.pi * 180)
	shot:loadTexture("media/material/floor_texture.png")
	alpha:addEntity(shot, SHAPE_PLANE, 0.0)
	shot.born = ticks
	shots:push(shot)
end
function playerShot(pos, angle, is_initial) -- Attempt to fire a gun from a player perspective
	if cm.just_beat_boss then return end
	local hitresult = a(bridge:getFirstEntityInRay(s, pos, vAdd(pos, a(bridge:rotateXzByAngle(a(Vector3(0,0,-km.strafe*100)), angle)))))
	local hit = hitresult and hitresult:entity()
	hit = enemy(hit)
	newShot(pos, a(hitresult:position()), true, is_initial)
	if hit and hit.proto then
		hit.hp = hit.hp - 1
		if hit.hp > 0 then
			newAlert( hitMsg(hit.proto) )
			hit.flashAt = km.enemy_flash_for
		else
			local epos = getPosition(hit)
			newAlert( killMsg(hit.proto) )
			eliminate(hit)
			gm.player_hp = clamp(0, gm.player_hp + 1, km.max_hp)
			if hit.proto.boss and not gm.won_whole_game then
				local youToThem = vMult(vNormalize(vSub(playerPos, epos)), -2) -- FIXME: BAD GLOBAL USE
				local box = ScenePrimitive(TYPE_BOX, 0.25,0.25,0.25)
				vSetPosition(box, epos)
				box:setColor(1,1,1,1)
				box:setTexture(texForLetter("%"))
				s:addPhysicsChild(box, SHAPE_BOX, 1.0)
				youToThem.y = 1
				s:setVelocity(box, youToThem)
				bridge:setAngularVelocity(s:getPhysicsEntityBySceneEntity(box), a(Vector3(5,5,0)))
				box.droppedAt = ticks
				table.insert(amulet, box)
				newAlert(string.format("The %s has dropped something.",hit.proto.name))
			end
		end
		s_uhit:Play()
		
		if hypershots then -- If using hyper gun, shot regenerates
			-- FIXME: Will leak if hypershots contains items on close
			hypershots_next:push({vDup(pos), angle})
		end
	else
		s_umiss:Play()
	end
end
function runShots() -- Call on each turn, manages shots and shot animations
	if hypershots then
		while true do
			local reshot = hypershots:pop()
			if not reshot then break end
			playerShot(a(reshot[1]),reshot[2])
		end
		hypershots = hypershots_next
		hypershots_next = Queue()
	end
	
	while true do
		local k = shots:peek()
		if k and ticks - k.born > km.shotlife then
			shots:pop()
			alpha:removeEntity(k)
			delete(k)
		else
			break
		end
	end
end

-- Misc animations

local stickyrandom_last = -2 local stickyrandom_value -- Random number that switches every other frame
function stickyrandom()
	if ticks - stickyrandom_last >= 2 then 
		stickyrandom_value = (math.random(2) == 1)
		stickyrandom_last = ticks
	end
	return stickyrandom_value
end

function runSpecial() -- Call on each turn, manages outro, death, win animations
	if cm.outro then
		local outrolen = km.outrolen * (cm.just_beat_boss and 8 or 1)
		local progress = cm.outro/outrolen

		cm.camera_height_mod = -progress
		m_moving:setPitch(1-progress)
		
		if gm.killed_by then
			cm.outro_fader:setColor(1.0,0.0,0.0,progress)
			--m_ambient:setVolume(1+progress*10)
			m_ambient:setPitch(1-progress*0.5)
			cm.deathPitch = progress*90
		elseif gm.won_whole_game then
			local brightness
			if cm.just_beat_boss then
				brightness = (1-cos(progress*math.pi*5))/4+progress/2
				cm.camera_height_mod = clamp(0, progress*5 - 4, 1)
			else
				brightness = progress
				cm.camera_height_mod = progress
			end
			cm.outro_fader:setColor(1.0,1.0,1.0,brightness)
			m_ambient:setPitch(1+progress*3)
			m_moving:setPitch(1+progress*3)
			m_panic:setPitch(1+progress*3)
			if cm.just_beat_boss then newAlert(got_amulet_message) end
		else
			cm.outro_fader:setColor(0.0,0.0,0.0,progress)
			m_ambient:setPitch(1-progress)
		end
		
		cm.music_mod = (1-progress)
		
		cm.outro = cm.outro + 1
		if cm.outro >= outrolen then
			cm.win_now = true
		end
	end
end

-- Music variation

function clamp(low, v, high)
	if low > v then return low end
	if high < v then return high end
	return v
end

class "MusicControl"

function MusicControl:MusicControl(song,spec)
	pull(self, {music_los = 0, nearest_prox = 10000, any_los = false,
		song=song})
	pull(self, spec)
end

function MusicControl:updateMusic()
	local los_dir = self.any_los and 1 or -1
	if not self.music_tested then self.music_tested = true self.music_los = los_dir end
	self.music_los = clamp(0, self.music_los + los_dir * km.los_ramp, 1)
	local prox_target = 0
	if self.nearest_prox < km.prox_bubble_outer then 
		prox_target = clamp(0, 1.0-(self.nearest_prox - km.prox_bubble_inner)/(km.prox_bubble_outer/km.prox_bubble_inner), 1.0)
	end
	local music_volume = (prox_target + self.music_los)*cm.music_mod*0.5
	self.song:setVolume(music_volume)
	if not self.music_ever and music_volume > 0 then
		self.music_ever = true
		self.song:Play(true)
	end
end
control_moving = MusicControl(m_moving)
control_panic = MusicControl(m_panic)

-- Build map (real)

level = levelStack[gm.at_level] -- Only build each level once per game, then remember it
if not level then
	level = LevelMap()
	level:construct()
	levelStack[gm.at_level] = level
end
level.theme:makeSky()
level:add(s,enemies, gm.won_whole_game)

-- Camera

local upright = r(Vector3(0,1,0))
function updateCamera() -- Call on each turn, manages... camera.
	local camera = s:getDefaultCamera()
	local at = getPosition(player)
	for i,camera in ipairs({s:getDefaultCamera(), alpha:getDefaultCamera()}) do
		if cm.mouseLooking then
			if gm.won_whole_game then at.y = 2 - at.y end
			camera:setPosition(at.x, at.y+cm.camera_height_mod ,at.z)
			camera:setYaw(cm.angle)
			camera:setPitch(cm.deathPitch)
			camera:setRoll(0)
		else
			local offs = a(Vector3(3,3,0))
			local at2 = vAdd(at, offs)
			camera:setPosition(at2.x,at2.y,at2.z)
			camera:lookAt(at,upright)
		end
	end
end
updateCamera()

-- Set up input handler

down = {} pressed = {}
mouseDownAt = nil
mouseAt = nil

class "Input" (EventHandler)
function Input:Input()
	self:EventHandler()
end

function cdelete(e)
	if e then delete(e) end
end

function Input:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == EVENT_MOUSEMOVE then
			cdelete(mouseAt) mouseAt = inputEvent:getMousePosition()
		elseif e:getEventCode() == EVENT_MOUSEDOWN then
			cdelete(mouseDownAt) mouseDownAt = inputEvent:getMousePosition()
			cdelete(mouseAt) mouseAt = vDup(mouseDownAt)
			down[inputEvent.mouseButton] = true
			pressed[inputEvent.mouseButton] = true
		elseif e:getEventCode() == EVENT_MOUSEUP then
			down[inputEvent.mouseButton] = nil
		elseif e:getEventCode() == EVENT_KEYDOWN then
			down[key] = true
			pressed[key] = true
		elseif e:getEventCode() == EVENT_KEYUP then
			down[key] = false
		end
	end
end

do
	input = Input()

	Services.Core:getInput():addEventListener(input, EVENT_MOUSEMOVE)
	Services.Core:getInput():addEventListener(input, EVENT_MOUSEDOWN)
	Services.Core:getInput():addEventListener(input, EVENT_MOUSEUP)
	Services.Core:getInput():addEventListener(input, EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, EVENT_KEYUP)
end