-- On Load

gm = {score=0}
km = {dxy = 1}
cm = {yo = 0, okay = true}

-- 34 x 24 is "round"
gfx_width = 35
gfx_height = 23
gfx_floor = 23
started = false

km.sfloor = gfx_height/4

dos = type_automaton()
dos:insert()
dos.g = dos:toGfx()
dos.g.fuzzy = true
dos.g:init(gfx_width, gfx_height)

play_width = 12
play_height = 8

-- BLOTCH
function setb(c, x, y)
	local atx, aty = x*3,y*3
	for drx=0,1 do
		for dry=0,1 do
			dos.g:pxpaint(atx+drx, aty+dry, c)
		end
	end
end

-- BLOTCH BETWEEN
function setbt(c, x, y, tx, ty)
	local isx = math.abs(tx-x) == 1
	local isy = math.abs(ty-y) == 1
	if isx and isy then
	elseif isx then
		local dry = y*3
		local drx = math.min(x,tx)*3+2
		dos.g:pxpaint(drx, dry, c)
		dos.g:pxpaint(drx, dry+1, c)
	elseif isy then
		local drx = x*3
		local dry = math.min(y,ty)*3+2
		dos.g:pxpaint(drx, dry, c)
		dos.g:pxpaint(drx+1, dry, c)
	end
end

function drawbg(x,y,dx,dy)
	x = x or 0
	y = y or 0
	dx = dx or gfx_width
	dy = dy or gfx_floor
	
	-- Note: Background fill should be in screen coords, drawing in absolute coords
	dos.g:pxfill(0xFF000000, x, y+cm.yo, dx, dy)
	player:paint()
	setb(0xFF00FFFF, pellet[1], pellet[2])
end

function playAt()
	return (gm.score+2)%4 == 0 and 0 or 2
end

class "Char"
function Char:Char(x,y)
	self.x = x
	self.y = y
	history = Queue()
	history:push({self.x,self.y})
	self.allowed = 1
end
function Char:check(x,y,notail)
	local tailclip = notail and 2 or 1
	for c=tailclip,history.count do
		local p = history[history.low + c - 1]
		if x == p[1] and y == p[2] then
			return true
		end
	end
	return false
end
function Char:paint()
	for c=1,history.count do
		local p = history[history.low + c - 1]
		setb(0xFF0000FF, p[1], p[2])
		if c < history.count then
			local p2 = history[history.low + c] -- + 1 effectively
			setbt(0xFF0000FF, p[1],p[2], p2[1], p2[2])
		end
	end
end
function Char:try(d,a)
	if d == 0 then return end
	local x,y = self.x, self.y
	if a == "x" then x = (x + d) % play_width
	elseif a == "y" then y = (y + d) % play_height
	end 
	
	if self:check(x,y,(history.count + 1 > self.allowed)) then 
		au.piano:setPitch(tk.pitchFrom(playAt())/2)
		au.piano:Play(false)
		cm.okay = false
		return
	end
	cm.okay = true
	
	self.x = x self.y = y
	
	if self.x == pellet[1] and self.y == pellet[2] then
		au.piano:setPitch(tk.pitchFrom(playAt()))
		au.piano:Play(false)
		gm.score = gm.score + 1
		self.allowed = self.allowed + 3
		makepellet()
	end
	
	history:push({self.x,self.y})
	while history.count > self.allowed do history:pop() end
	drawbg()
end

player = Char(3,3)

function makepellet()
	while true do
		pellet = {math.random(0,play_width-1), math.random(0,play_height-1)}
		if not player:check(pellet[1],pellet[2]) then
			break
		end
	end
end
makepellet()

drawbg()

-- Sound
basstimer = MiniTimer(0.1)
km.steps = {2,1,2,2,1}
function bass()
	local note = math.random(-1,3)
	local split = (cm.wasokay and not cm.okay) or (cm.okay and not cm.wasokay)
	if split then
		note = -12
	end
	if not split and (not cm.okay or note < 0) then
		au.tone:Stop()
	else
		local at = 0
		for i=0,note do
			print({note, km.steps})
			at = at + km.steps[(note % #km.steps) + 1]
		end
		au.tone:setPitch(tk.pitchFrom(note))
		if not au.tone:isPlaying() then
			au.tone:Play(true)
		end
	end
	cm.wasokay = cm.okay
end
bass()

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
			down[key] = nil
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