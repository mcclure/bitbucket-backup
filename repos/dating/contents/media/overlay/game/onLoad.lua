-- On Load

-- DON'T FORGET TO COMPILE THIS ONE WITH -O3! (?)

pull(gm, {r=Router():insert(), au={}, gfx={}})

Services.Renderer:setClearColor(0,0,0,0)

killDos()
dos = type_automaton()
dos:insert()
dos.g = dos:toGfx()

local ls = LevelScreen({channel=v, g=dos.g}):insert()
gm.ls = ls

-- make areas

--local a = ls:loadNext( LevelCanvas({g=still_star()}), {} ) -- DELETE

-- OMGHAX
local sfx = LevelCanvas({g=r(gfxcontainer(32,64))})
local sfx_at = ls:loadNext(sfx, {} )
local sfx2 = LevelCanvas({g=r(gfxcontainer(32,64))})
local sfx2_at = ls:loadNext(sfx, {} )

local street = LevelCanvas({g=make_image("street", "bg")})
local street_at = ls:loadNext( street, {float=true} )

local darkstreet = make_image("street-dark", "bg") -- We'll need this later

ls.bg[street_at]:get(4,3).onClose = function() -- awning support
	local grid = gm.ls.bg[street_at]
	grid:set(5,3, grid:get(5,4))
	grid:set(5,4, nil)
	gm.ls.umbrella[3] = 3
	gm.ls.umbrella[4] = 3
end

ls.bg[street_at]:get(4,4).onClose = function() -- awning left
	gm.ls.umbrella[3] = 5
end

ls.bg[street_at]:get(5,4).onClose = function() -- awning right
	gm.ls.umbrella[4] = 5
end

ls:watermark_key_all("building",street_at,2)

ls:loadAt(street_at, LevelCanvas({g=make_image("date", "bg")}), {xoff=1} )

local scape_at = ls:loadNext( LevelCanvas({g=make_image("scape", "bg")}), {float=true, rainy=true} )

local streetback_at = ls:loadNext( LevelCanvas({g=make_image("streetback", "bg")}), {rainy=true} )

ls:loadAt(streetback_at, LevelCanvas({g=still_star()}), {immortal=true})

local purples = {low=3,high=4}

ls:watermark_key_all("purple",streetback_at,nil, 4+purples.low)

local star = StarCanvas({height=4})
ls:loadAt(streetback_at, star, {immortal=true})

ls:watermark_key_all("purple",streetback_at,nil, 8+purples.high)

local vortex = VortexCanvas({height=4})
ls:loadAt(streetback_at, vortex, {immortal=true})

gm.gfx.rain = RainCanvas()

-- Make sounds

local pbase = 21.25
local ppurple = 17.50
local pbuilding = 32.50
local pend = 150

gm.au.rain = r(BSound())
gm.au.rain:setPitch(6.5)
gm.au.rain:setParam(0,pbase)
gm.au.rain:setParam(1,1)

gm.au.crash = r(WhiteSound())
gm.au.crash:setPitch(50)
gm.au.crash:setVolume(0)

gm.au.rain:Play(true)
gm.au.crash:Play(true)

local hisser = Ent({onTick=function(self)
	local set = nil
	if gm.ls.watermark.building and gm.ls.watermark.building>1 then
		set = pbuilding
	elseif gm.ls.watermark.purple then
		if gm.ls.watermark.purple==purples.low then
			set = (pbase + ppurple) / 2
		elseif gm.ls.watermark.purple==purples.high then
			set = ppurple
		end
	end
	if set then 
		gm.au.rain:setParam(0,set)
	end
end}):insert()

-- Setup "ending". This gets dense, I'll comment heavily.

gm.doomed = { star.ent, vortex.ent }

-- Store a copy of the screen's initial state in this table.
local worldbackup = {}
for i,v in ipairs(gm.ls.bg) do
	table.insert(worldbackup, v:clone())
end

-- Timing constants for various effects below.
local crashy = {to=60*4, every=4}
local restore = {at=60*1, every=10}
local clicky = {from=60*4.5, to=60*4.5+4, pitch=5}
local fadeout = {from=60*6, to=60*15}
local finally = {at=60*20}
local floaty = {float=true}

-- Map a "tile" from sfx canvas to sfx layer. (fade)
function sfxmap(fx, fy, tx, ty)
	local pxat = gm.ls:rebase(P(fx,fy-(2-km.lh))) -- FIXME Awful hack!
	sfx:loadOne(gm.ls, gm.ls.bg[sfx_at], tx, ty, sfx_at, P(tx,ty), pxat, floaty)
end

-- Map a "tile" from sfx canvas to sfx layer. (person)
function sfx2map(fx, fy, tx, ty)
	local pxat = gm.ls:rebase(P(fx,fy-(2-km.lh))) -- FIXME Awful hack!
	sfx2:loadOne(gm.ls, gm.ls.bg[sfx2_at], tx, ty, sfx2_at, P(tx,ty), pxat, floaty)
end

local startflipping = nil

-- Starting from the moment you tap the guy's head:
gm.ls.bg[street_at]:get(2,12).onTap = function()
	hisser:die() -- Tiles: Stop effecting rain.
	gm.p.mute = true -- PC: Stop effecting rain.
	Ent({crashoff = 0, onTick=function(self) -- Start this loop until program terminate:
		if not self.born then -- Init
			self.born = ticks
			gm.au.rain:setParam(0,pend)
		end
		local time = ticks-self.born
		
		-- First thing: Crash audio should blow up and hiss downward in a crazy warble.
		if time < clicky.from then
			local crashprogress = math.min(time/(crashy.to),1)
			if crashprogress < 1 then
				if 0==time%crashy.every then
					self.crashoff = math.random(-1,1)
				end
				gm.au.crash:setPitch(5+crashprogress*10+self.crashoff)
			end
			gm.au.crash:setVolume(1-crashprogress)
		else -- then later, come back with a single click.
			if time == clicky.from then
				street.g:pxcopy(darkstreet)
				gm.au.crash:setPitch(clicky.pitch)
			end
			local clickprogress = math.min((time-clicky.from)/(clicky.to-clicky.from),1)
			gm.au.crash:setVolume(1-clickprogress)
		end
		
		-- Scramble for a moment. Notice the ticks vs time metric switch here! This is intentional.
		if startflipping and ticks >= startflipping and time < restore.at then
			for i,v in ipairs(gm.ls.bg) do
				if i>2 and math.random(1,7) <= 3 then
					local x1,x2=math.random(1,km.lw),math.random(1,km.lw)
					local y1,y2=math.random(2,km.lh),math.random(2,km.lh)
					local temp = v:get(x1,y1)
					local temp2 = v:get(x2,y2)
					v:set(x1,y1,temp2)
					v:set(x2,y2,temp)
				end
			end
		end
		
		-- After a few seconds, rewrite the world from original backup, one row at a time.
		if time >= restore.at and 0==(time-restore.at)%restore.every then
			local y = (time-restore.at)/restore.every
			if y <= km.lh then
				y = km.lh-y
				for id,from in ipairs(worldbackup) do
					local to = gm.ls.bg[id]
					for x=1,km.lw do
						to:set(x,y,from:get(x,y))
					end
				end
				
				-- While we're at it, put the sfx layer(s) in place. This means the "blank" tile for sfx 1,
				for x=1,km.lw do
					sfxmap(1,1,x,y)
				end
				
				-- For sfx 2 it means moving the two redhead tiles into place.
				if y == 3 then
					sfx2.g:pxcopy(gm.p.sprites["11"], 0, 0, 0, 0, -1, 32)
					sfx2map(1,2,8,y)
				end
				if y == 2 then
					sfx2.g:pxcopy(gm.p.sprites["11"], 0, 32, 0, 32, -1, 32)
					sfx2map(1,1,8,y)
				end
				
				-- If we just finished the whole thing, we can switch off the outer-space animations (they're invisible)
				if y == 1 then
					for i,v in ipairs(gm.doomed) do v:die() end
				end
				
				-- And if we just hit the awning then fix that
				if y == 4 then
					gm.ls.umbrella=tableCopy(km.original_umbrella)
				end
			end
		end
		
		-- Finally: 
		if time>fadeout.from then
			local extent = math.min( (time-fadeout.from)/(fadeout.to-fadeout.from), 1.0 )
			local rgb = rgbcode(0,0,0,extent) -- Figure out the exact gray we're using. Then crazy flipping to avoid needing an extra layer:

			-- Now that the redhead's done, clean out the blank cell with the base alpha color.
			sfx.g:pxfill(rgb)

			local rainextent = math.min( (time-fadeout.from)/(finally.at-fadeout.from), 1.0 )
			gm.au.rain:setParam(1,1+9*rainextent)
		end
		
		if time>finally.at then
			gm.needexit = bridge.fake
		end
	end}):insert()
end
gm.ls.bg[street_at]:get(2,12).onClose = function(self)
	-- We've just finished destroying the dude's head, so remove the redhead ent and replace with identical but static redhead art
	gm.p.invisible = true
	sfx2.g:pxcopy_blend(gm.p.sprites["11"], 0, 0, 0, 0, -1, -1, gm.p.xflip) -- blend looks wrong but i need it for xflip
	sfx2map(1,1,  2,2) -- the sfx2 layer was never given tiles...
	sfx2map(1,1+1,2,3)
	startflipping=ticks+1
end

-- Make player

gm.p = PlayerController({pos=P(5,3),live=ls}):insert()

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
		if not down[key] then
			down[key] = true
			pressed[key] = true
		end
		for k,v in pairs(listeners) do
			v:push({key, bridge:charCode(inputEvent)})
		end
	elseif e:getEventCode() == InputEvent.EVENT_KEYUP then
		down[key] = nil
	end
end

do
	input = Input()

--	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEMOVE)
--	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEDOWN)
--	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEUP)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, input.handleEvent, InputEvent.EVENT_KEYUP)
end