-- On Load
memory_setup()

ticks = 0 -- whatever

function clamp(x,y,z)
	return math.max(x,math.min(y,z))
end

-- Set up visuals

do
	bfile = a(bridge:loadVoxel("media/vox/scenetest1.png"))
	background = r(Vox(bfile.xdim+16,bfile.ydim,bfile.zdim+8)) -- SPECIAL KNOWLEDGE OF PNG FILE
	background:clear()
	background:blit(bfile, 8,0,0)
	background:blit(bfile, 0,0,0, 0,0,0, 8,8) -- Copy left over
	background:blit(bfile, background.xdim-8,0,0, bfile.xdim-8,0,0, 8,8) -- Copy right over
	background:clear(4,0,0, 4,8,17) background:blit(bfile, 4,0,0, 0,0,0, 4,8,17) -- Patch hole on left 
	background:clear(background.xdim-8,0,0, 4,8,17) background:blit(bfile, background.xdim-8,0,0, bfile.xdim-4,0,0, 4,8,17) -- Patch hole on left 
	background:blit(background, 0,0,background.zdim-8, 0,0,background.zdim-16, background.xdim,background.ydim,8) -- Copy down over
end

board = Vox(8,8,8)
cube = r(Cuber(board))
walk = { r(bridge:loadVoxel("media/vox/walk1.png")), r(bridge:loadVoxel("media/vox/walk2.png")) }
font = r(bridge:loadVoxel("media/5tall.png"))
dissolve = {}
for z=1,8 do 
	local lz = {} for y=1,8 do
		local ly = {} for z=1,8 do 
			table.insert(ly,false)
		end table.insert(lz,ly)
	end table.insert(dissolve,lz)
end
--	deleteme = ScenePrimitive(TYPE_BOX)	cube:addEntity(deleteme) -- Test object

bridge:setSceneClearColor(cube,0,229,255,0)

-- "Title screen"

function killDos() if dos then dos:die() dos = nil end end
dos = type_automaton()
dos:insert()
dos:set(0,1,"ARROW.KEYS.TO.MOVE       DRAG.FOR.CAMERA")

-- Music

if not music then music = Sound("media/bureaucratic_nightmare.ogg") end
if not steps then steps = {Sound("media/step1.ogg"), Sound("media/step2.ogg")} end
if not stepstop then stepstop = Sound("media/stop.ogg") end
for i,sound in ipairs(steps) do
	sound:setVolume(0.5)
end
stepstop:setVolume(0.5)

-- Font

fontmetric = {}
do 
	local ybar = font.ydim-1
	local xstart = nil
	local xlen = nil
	function flush() if xstart then table.insert(fontmetric, {xstart=xstart,xlen=xlen}) xlen = nil xstart = nil end end
	for x=0,font.xdim-1 do
		if a(font:get(x, ybar)).a > 0 then
			if not xstart then xstart = x xlen = 0 end
			xlen = xlen + 1
		else flush() end
	end
	flush()
end 
textqueue = Queue()
function pushText(text)
	local newtext = {scroll=0,blocks={}}
	local xoff = 0
	for i = 1,#text do
		local a = text:byte(i)
		if a >= 33 and a <= 127 then
			local am = fontmetric[a-32]
			table.insert(newtext.blocks, {xto=xoff, am=am})
			xoff = xoff + am.xlen
		end
		xoff = xoff + 1
	end
	newtext.xlen = xoff
	textqueue:push(newtext)
end

-- Move machine: Camera, Movement, Player, Konstants
cm = {xang = -25, yang = -25}
mm = { move=0, zaxis=false, vzaxis=false, blocked = false, mab=0, jump=false, laststill=0, stepstaken=0, musicspeed=0.0, tempmove = nil, endgate=false, }
km = { canclimb=2, tall=4, startx=12, starty=1, startz=background.zdim-12, }
pm = { camx=-4, camy=0, camz = -4, x=km.startx, y=km.starty, z=km.startz, }

function modget(b, x, y, z)
	if x < 0 or y < 0 or z < 0 or x >= b.xdim or y >= b.ydim or z >= b.zdim then
		return {a=0}
	end
	return a(b:get(x, y, z))
end
function mapcheck(x,y,z)
	return modget(background,x,y,z).a > 0
end
function mapheight(x,z)
	local y = 0
	while true do
		if y < background.ydim and mapcheck(x,background.ydim-y-1,z) then 
			y = y + 1
		else
			break
		end
	end
	return y
end

function walksafe(dx,dz)
	for dy = km.canclimb,km.tall-1 do
		if mapcheck(pm.x+dx,background.ydim-(pm.y+dy)-1,pm.z+dz) then return false end
	end
	return true
end

function block()
	if mm.endgate then return nil end
	if not mm.blocked or (mm.mab ~= mm.move and mm.move ~= 0) then
		mm.blocked = true
		mm.mab = mm.move
		stepstop:Play(false)
	end
end

function updateBoard()
	-- Cut "action" framerate -- TODO: What if framerate drops?
	if 0 ~= ticks%5 then return end
	local frame = math.floor(ticks/5)
	local walkframe = 0
	
	-- Manage walking state
	local walking = mm.move ~= 0
	if walking then killDos()
	else mm.laststill = frame end
	local walkframe = (frame-mm.laststill) % 2

	-- Manage movement
	if mm.move ~= 0 and not mm.endgate then
		if mm.zaxis then
			if walksafe(0,mm.move) and walksafe(0,mm.move*2) then
				pm.z = pm.z + mm.move
				mm.blocked = false
				mm.vzaxis = true -- vzazis = visible zaxis
			else block()
			end
		else
			if walksafe(mm.move,0) and walksafe(mm.move*2,0) then
				pm.x = pm.x + mm.move
				mm.blocked = false
				mm.vzaxis = false
			else block()
			end
		end
	end
	if pm.z <= 13 then -- SPECIAL KNOWLEDGE OF PNG FILE
		if pm.x <= 3 then pm.x = pm.x + 4 end
		if pm.x >= background.xdim-3 then pm.x = pm.x - 4 end
	else
		if pm.x <= 4 then pm.x = pm.x + 8 end
		if pm.x >= background.xdim-4 then pm.x = pm.x - 8 end
	end
	if pm.z >= background.zdim-4 then pm.z = pm.z - 8 end
	
	pm.y = mapheight(pm.x, pm.z)
		
	-- Walking "sound"
	if walkframe == 0 and not mm.blocked and not mm.endgate then
		if walking then
			steps[(mm.stepstaken%2)+1]:Play(false)
			mm.stepstaken = mm.stepstaken + 1
		end
		mm.tempmove = mm.move
		mm.move = 0
	end
		
	-- Redraw board
	board:clear()
	local scrolly = -pm.camy
	for i,xoff in ipairs({0}) do
		for i,zoff in ipairs({0}) do
			local scrollx = xoff-(pm.x + pm.camx)
			local scrollz = zoff-(pm.z + pm.camz)
			board:blit(background, scrollx,scrolly,scrollz)
		end
	end
	local walkvox = walk[mm.blocked and 1 or (walkframe + 1)]
	board:xzblit(true, mm.vzaxis, walkvox, -pm.camx, board.xdim-(walkvox.ydim + pm.y + pm.camy), -pm.camz)
		
	-- Special event (ending)
	if mm.endgate then
		for x = 1,8 do for y=1,8 do for z=1,8 do
			if dissolve[x][y][z] then board:clear(x-1,y-1,z-1, 1,1,1) end
		end end end
	end
		
	-- Draw any messages -- Note: Whole thing here assumes zdim=xim
	local msg = textqueue:peek()
	if msg then
		-- if msg.vzaxis == nil then msg.vzaxis = mm.vzaxis end -- Make direction "sticky"
		for i,block in ipairs(msg.blocks) do
			local tx = board.xdim-1
			local tz = board.zdim-1
			local off = -msg.scroll + block.xto
			if msg.vzaxis then tz = tz + off tz = board.zdim - tz - 1 else tx = tx + off end
			board:xzblit(false, msg.vzaxis or false, font, tx, 1, tz, block.am.xstart, 0, 0, block.am.xlen, font.ydim-2, 1)
		end
		msg.scroll = msg.scroll + 1
		if msg.scroll > msg.xlen + board.zdim then textqueue:pop() end
	end
end
updateBoard()

function updateMusic()
	local tempmove = mm.move if mm.tempmove then tempmove = mm.tempmove mm.tempmove = nil end
	local musicoff = (tempmove ~= 0 and not mm.blocked and not mm.endgate) and 0.04 or -0.04
	local new_musicspeed = clamp(0.1, mm.musicspeed+musicoff, 1.0)
	if mm.musicspeed ~= new_musicspeed then
		mm.musicspeed = new_musicspeed
		music:setPitch(mm.musicspeed)
	end
	
	-- Handle special "events"... TODO: This is a dumb place to do this. -- SPECIAL KNOWLEDGE OF PNG FILE
	if not mm.endgate and (pm.x <= 18 and pm.z <= 9 and pm.x >= 12 and pm.z >= 3) and mm.musicspeed < 0.5 then
		mm.endgate = true
		pushText("                TO BE CONTINUED")
	end
	if mm.endgate then
		for y=1,10 do
			local x = math.random(1,8) local y = math.random(1,8) local z = math.random(1,8)
			dissolve[x][y][z] = true
			if z<board.zdim-1 then board:clear(x-1,y-1,z-1, 1,1,1) end
		end
	end

end
updateMusic()
music:Play(true)

function updateCamera()
	if cameraRot then a(cameraRot) end -- safe to a(nil)?
	local q1 = a(Quaternion(1,0,0,0)) local q2 = a(Quaternion(1,0,0,0))
	if _DEBUG then print(string.format("x %s y %s", cm.xang, cm.yang)) end
	q1:fromAxes(0,cm.xang,0)
	q2:fromAxes(0,0,cm.yang)
	cameraRot = bridge:qmult(q1,q2)

	local from = a(Vector3(10,10,10))
	from = a(bridge:qapply(cameraRot, from))
	local camera = cube:getDefaultCamera()
	camera:setPosition(from.x,from.y-1,from.z)
	camera:lookAt(a(Vector3(0,-1,0)),a(Vector3(0,1,0)))
end
updateCamera()

-- Set up input handler
down = {}
mouseDownAt = Vector3(0,0,0)
mouseAt = Vector3(0,0,0)

class "Input" (EventHandler)
function Input:Input()
	self:EventHandler()
end

function Input:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == EVENT_MOUSEMOVE then
			delete(mouseAt) mouseAt = inputEvent:getMousePosition()
		elseif e:getEventCode() == EVENT_MOUSEDOWN then
			delete(mouseDownAt) mouseDownAt = inputEvent:getMousePosition()
			delete(mouseAt) mouseAt = vDup(mouseDownAt)
			down[inputEvent.mouseButton] = true
		elseif e:getEventCode() == EVENT_MOUSEUP then
			down[inputEvent.mouseButton] = nil
		elseif e:getEventCode() == EVENT_KEYDOWN then
			down[key] = true
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
