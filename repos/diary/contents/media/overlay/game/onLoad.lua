-- On Load

gm = {}
km = {dxy = 1}
cm = {yo = 0}

gfx_width = 280
gfx_height = 160
gfx_floor = 192
started = false

km.sfloor = gfx_height/4

dos = freetype_automaton()
dos:insert()
dos.g = dos:toGfx()

map.fore_g = r(gfxcontainer())
map.fore_g:load_image(map.fore)
map.back_g = r(gfxcontainer())
map.back_g:load_image(map.back)

level_width = clamp(0,gfx_width,map.fore_g.w)

function drawbg(x,y,dx,dy)
	x = x or 0
	y = y or 0
	dx = dx or level_width
	dy = dy or gfx_floor
	
	-- Note: Background fill should be in screen coords, drawing in absolute coords
	dos.g:pxfill(0xFF000000, x, y+cm.yo, dx, dy)
	dos.g:pxcopy_blend(map.back_g, x, y, 0, 0, dx, dy)
	dos.g:pxcopy_blend(map.fore_g, x, y, 0, 0, dx, dy)
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
	self:enter()
end
function Char:try(x,a)
	local isx = a == "x"
	local na = isx and "y" or "x"
	local da = isx and "w" or "h"
	local dna = isx and "h" or "w"
	local got = 0
	local dir = x<0 and -1 or 1
	local side = x<0 and 0 or self[da]-1
	for ax=dir,x,dir do
		local tx = ax + self[a] + side
		local poison = false
		for ay=0,self[dna]-1 do
			local ty = ay + self[na]
			local test
			if isx then
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

player = Char("media/char/stand.png")
player.x = 3 player.y = 3
player:enter()

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