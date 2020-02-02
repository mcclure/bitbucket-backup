-- On Load

-- Base setup
pull(km, {boxsize=1, vzero = Vector3(0,0,0), adhocui = _DEBUG, baseface=Vector3(1,0,0), altface=Vector3(0,0,1), faceup=Vector3(0,1,0), sensitive=0.5, levelcount=4, sweepFrames=8})
pull(gm, {cam={x=0,y=28}, clicks={}, boxes={}, flags={}})

Services.Core:enableMouse(false)

tm.clearColor()

gm.s = r(CollisionScene(Vector3(20,20,20)))
gm.s.ownsChildren = true

-- Shaders

gm.s:getDefaultCamera():setPostFilterByName("Blur")
shaders = {}
shaders[1] = shaderBindings(gm.s:getDefaultCamera(),
					{blurSize=1/256.0}, 0)
shaders[2] = shaderBindings(gm.s:getDefaultCamera(),
					{blurSize=1/256.0, brighten=tm.gfx.brighten, thresh=tm.gfx.thresh}, 1)
newblur("blur", tm.gfx.blur)

if not tm.demoMode then
gm.pulser = tm.pulser({target=gm.flags})
end


-- Construct level

function randAround(point, span, power)
	local r = (math.random()-0.5)*2
	if power then r = math.pow(r, power) end
	return point+r*span
end

local collider = ScenePrimitive(ScenePrimitive.TYPE_BOX, .5,.5,.5)
collider.visible = false
gm.s:addCollisionChild(collider, CollisionSceneEntity.SHAPE_SPHERE)
gm.collider = collider

for i=1,tm.howmany() do
	local box =  ScenePrimitive(ScenePrimitive.TYPE_BOX, 1,1,1)
	tm.boxcolor(box, i)
	tm.boxposition(box, i)
	gm.s:addCollisionChild(box, CollisionSceneEntity.SHAPE_BOX)

	table.insert(gm.boxes, box)
end

gm.s:Update() -- It doesn't know where anything is yet
	
-- Setup initial camera

function resetCamera()
	local at = gm.forceCamera and gm.forceCamera or standOn(gm.on)
	
	vSetPosition(gm.s:getDefaultCamera(), at)	
	gm.s:getDefaultCamera():setYaw(-gm.cam.x)
	gm.s:getDefaultCamera():setPitch(-gm.cam.y)
end

gm.start = findIdeal(gm.boxes, tm.startSort)
gm.on = gm.start
resetCamera()
	
-- Construct flagpole / ending

function plant(box)
	local boxcenter = box:getPosition()
	local color = Color(255,0,0,255)

	local obj = {focus=box, onClick = function(self) -- ENDING
		pushInput()
		local boxPosition = self.focus:getPosition()
		local focusPosition = vAdd(boxcenter, Vector3(0,km.boxsize,0)) -- Position of cylinder
		local cameraPosition = vAdd(boxcenter, Vector3(0,km.boxsize+0.2,km.boxsize*2))
		local camera = gm.s:getDefaultCamera()
		vSetPosition(camera, cameraPosition)
		camera:lookAt(focusPosition, km.faceup)
		killDos() dos = type_automaton() dos:insert()
		gm.pulser:mute()
		
		local callouts = {
			"#PLEASE#LEAVE#ME#ALONE.#",
			"#I#DON'T#WANT#TO#TALK#TO#YOU.#",
			"#GO#AWAY.#",
			"#STAY#AWAY#FROM#ME.#",
		}
		dos:set_centered(0,23,40,callouts[fm.level])
		gm.snd:setIparam(1, 81920)
		TimedEnt({onInput = function(self)
			if self:ticks()>(FPS/2) and pressed[0] then
				if fm.level < km.levelcount then
					fm.level = fm.level + 1
					bridge:rebirth()
				else
					bridge:load_room_txt("media/exit.txt")
				end
			end
		end}):insert()
	end}
	local function register(e)
		gm.clicks[ bridge:bindingId(e) ] = obj
	end
	local function setup(e)
		bridge:setColorObj(e, color)
		gm.s:addCollisionChild(e, CollisionSceneEntity.SHAPE_BOX)
		table.insert(gm.flags, e)
		register(e)
	end
	
	register(box)

	local body = ScenePrimitive(ScenePrimitive.TYPE_CYLINDER, km.boxsize, km.boxsize/8,8) -- , segments
	vSetPosition(body, vAdd(boxcenter, Vector3(0,km.boxsize,0)))
	setup(body)
	
	local head = ScenePrimitive(ScenePrimitive.TYPE_SPHERE, km.boxsize/4,8,16) -- , segments, segments
	vSetPosition(head, vAdd(boxcenter, Vector3(0,km.boxsize*1.5,0)))
	setup(head)
	
	local skirt = ScenePrimitive(ScenePrimitive.TYPE_CONE, km.boxsize/2, km.boxsize/4 + km.boxsize/16,16) -- , segments
	vSetPosition(skirt, vAdd(boxcenter, Vector3(0,km.boxsize*0.95,0)))
	setup(skirt)
end

local flagpole = findIdeal(gm.boxes, tm.flagpoleSort)
plant(flagpole)

-- Setup mouse input

function vEq(e,v)
	if not v and not e then return true end
	return v and e and e.x == v.x and e.y == v.y
end

if not tm.demoMode then

Ent({onInput = function(self)
	if mouseAt and not gm.freezeInput and not gm.animateFreezeInput then
		if not vEq(gm.lastMouseAt, mouseAt) then
			if gm.lastMouseAt then
				local diff = vSub(mouseAt, gm.lastMouseAt)
				gm.cam.x = gm.cam.x + diff.x*km.sensitive
				gm.cam.y = clamp(-70, gm.cam.y + diff.y*km.sensitive, 70)
				resetCamera()
				delete(gm.lastMouseAt)
			end
			if mouseAt.y > 3*surface_height/4 or mouseAt.y < surface_height/4 -- Mouse wraparound
				or mouseAt.x > 3*surface_width/4 or mouseAt.x < surface_width / 4 then
				Services.Core:warpCursor(surface_width/2,surface_height/2)
				gm.lastMouseAt = nil
			else
				gm.lastMouseAt = vDup(mouseAt)
			end
			gm.pulser:jolt()
		end

		if pressed[0] then
			local camera = gm.s:getDefaultCamera()
			local position = camera:getPosition()
			local matrix = camera:getConcatenatedMatrix()
			local ray = matrix:rotateVector(Vector3(0,0,-100))
			local face = vAdd(position, ray)
			local hit = bridge:getFirstEntityInRay(gm.s, position, face)
			if hit then
				local special = gm.clicks[ bridge:bindingId(hit) ]
				if special then
					special:onClick()
				else -- Default handler: Jump
					if boxCollide(hit) then
						gm.pulser:crash()
					else
						gm.pulser:animate(gm.on, hit)
						gm.on = hit
					end
				end
			else
				gm.pulser:miss()
			end
		end
	end
end}):insert()

end

if tm.demoEnt then
	tm.demoEnt:insert()
end

-- Sound

gm.snd = r(BSound())
for i,v in ipairs(tm.au.param) do gm.snd:setParam(i-1,v) end
for i,v in ipairs(tm.au.iparam) do gm.snd:setIparam(i-1,v) end
gm.snd:setVolume(tm.au.volume)
gm.snd:Play(true)