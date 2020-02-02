-- On Load

fm = fm or {}
fm.coeff = fm.coeff or 1
gm = {}
km = {dxy = 1}
cm = {yo = 0}
map = {
--	fore="media/level1/fore.png",
	back="media/back.png",
}

gfx_width = 280
gfx_height = 160
gfx_floor = 192
started = false

km.sfloor = gfx_height/4

function killDos() if dos then dos:die() bridge:unlinkgc(dos) dos = nil end end killDos()
dos = freetype_automaton()
dos:insert()
dos.g = dos:toGfx()

if map.fore then
	map.fore_g = r(gfxcontainer())
	map.fore_g:load_image(map.fore)
end
if map.back then
	map.back_g = r(gfxcontainer())
	map.back_g:load_image(map.back)
	if fm.invert then 
		map.back_g:pxcopy_invert(map.back_g)
	end
end

gm.graph = r(gfxcontainer())
gm.graph:init(280,192)
gm.graph:pxfill(0xFF000000)
gm.scratch = r(gfxcontainer())
gm.scratch:init(280,192)

level_width = clamp(0,gfx_width,map.fore_g and map.fore_g.w or 9000000)

function drawbg(x,y,dx,dy)
	x = x or 0
	y = y or 0
	dx = dx or level_width
	dy = dy or gfx_floor
	
	-- Note: Background fill should be in screen coords, drawing in absolute coords
--	dos.g:pxfill(0xFF000000, x, y+cm.yo, dx, dy)
	if map.back_g then dos.g:pxcopy(map.back_g, x, y, 0, 0, dx, dy) end
	if map.fore_g then dos.g:pxcopy_blend(map.fore_g, x, y, 0, 0, dx, dy) end
	if gm.graph   then dos.g:pxcopy_xor(gm.graph,     x, y, 0, 0, dx, dy) end
end
drawbg()

class "Char"
function Char:Char(img)
	self.g = r(gfxcontainer())
	self.g:load_image(img)
	self.x = 0
	self.y = 0
	self.w = self.g.w
	self.h = self.g.h
end
function Char:enter()
	dos.g:pxcopy_xor(self.g, self.x, self.y-cm.yo)
end
function Char:exit()
	--self:enter()
end
function Char:try(x,a)
	local isx = a == "x"
	local na = isx and "y" or "x"
	local da = isx and "w" or "h"
	local dna = isx and "h" or "w"
	local got = 0
	local dir = x<0 and -1 or 1
	local side = x<0 and 0 or self[da]-1
	local wh = {w=gfx_width, h=gfx_floor}
	for ax=dir,x,dir do
		local tx = ax + self[a] + side
		local poison = false
		for ay=0,self[dna]-1 do
			local ty = ay + self[na]
			local test
			if not map.fore_g then
				test = (tx < 0 or tx>=wh[da])
			elseif isx then
				test = map.fore_g:pxhas(tx,ty)
			else
				test = map.fore_g:pxhas(ty,tx)
			end
			if test then
				poison = true break
			end
		end
		if poison then break end
		got = got + dir
	end
	self[a] = self[a] + got
	return got
end

player = Char("media/stand.png")
player.x = 3 player.y = 3
player:enter()

gm.sound = r(BSound())
for _i=1,gm.sound:toneCount() do
	local i = _i-1
	gm.sound:setTone(6, i, fm.coeff)
end
gm.sound:Play()
function q() gm.sound:Stop() end

-- Set up input handler

down = {} pressed = {}
listeners = {} -- To get unicode-level key input
mouseDownAt = nil
mouseAt = nil

class "Input" (EventHandler)
function Input:Input()
	EventHandler.EventHandler(self)
end

function Input:handleEvent(e)
	local inputEvent = InputEvent(e)
	local key = inputEvent:keyCode()
	if e:getEventCode() == InputEvent.EVENT_MOUSEMOVE then
		cdelete(mouseAt) mouseAt = inputEvent:getMousePosition()
	elseif e:getEventCode() == InputEvent.EVENT_MOUSEDOWN then
		cdelete(mouseDownAt) mouseDownAt = inputEvent:getMousePosition()
		cdelete(mouseAt) mouseAt = vDup(mouseDownAt)
		down[inputEvent.mouseButton] = true
		pressed[inputEvent.mouseButton] = true
	elseif e:getEventCode() == InputEvent.EVENT_MOUSEUP then
		down[inputEvent.mouseButton] = nil
	elseif e:getEventCode() == InputEvent.EVENT_KEYDOWN then
		down[key] = true
		pressed[key] = true
		for k,v in pairs(listeners) do
			v:push({key, bridge:charCode(inputEvent)})
		end
	elseif e:getEventCode() == InputEvent.EVENT_KEYUP then
		down[key] = false
	end
end

do
	input = Input()

	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEMOVE)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEDOWN)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEUP)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYUP)
end