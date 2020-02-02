-- Pregame -- On Load

-- Util

local uiscale = surface_height/600
function X(x)
	return x * uiscale
end

-- File handling

function splitOne(str, token)
	local at = string.find(str, token)
	if not at then
		return str, nil
	else
		return string.sub(str, 1, at-1), string.sub(str, at+1)
	end
end

function loadIni(path, external)
	local result = {}
	local data = nil
	if external then data = bridge:filedump_external(path) else data = bridge:filedump(path) end
	while data do
		local current, after = splitOne(data, "[\r\n]")
		if current.sub(1,1) ~= "#" then
			local key, value = splitOne(current, "=")
			if value then
				result[key] = value
			end
		end 
		data = after
	end
	return result
end

function fallbackIniPath()
	return "o_o.ini"
end

function defaultIniPath()
	if _DEBUG then return "o_o.ini" end
	if _WINDOWS then return "..\\dat\\o_o.ini" end
	return "../dat/o_o.ini"
end

function defaultIni()
	return tableMerge(
		loadIni("media/default_savefile.txt"),
		loadIni(defaultIniPath(), true)
		)
end

local keys = {"MapX", "MapY", "Misc0", "Misc1", "Misc2", "Misc3", "PlayerName", "Switch0", "Switch1", "Switch2", "Switch3", "Switch4", "Switch5", "Switch6", "Switch7"}
local switches = {"Switch0", "Switch1", "Switch2", "Switch3", "Switch4", "Switch5", "Switch6", "Switch7"}

local params = {"MapX", "MapY", "Misc0", "Misc1", "Misc2", "Misc3"}
local paramScales = {MapX=1023, MapY=1023, Misc0=65535, Misc1=65535, Misc2=65535, Misc3=65535}

function saveIni(t, path)
	local result = "[Main]\n"
	for i,v in ipairs(keys) do
		result = string.format("%s%s=%s\n",result,v,t[v] or 0)
	end
	local success = bridge:filedump_external_out(defaultIniPath(), result)
	if not success then
		bridge:filedump_external_out(fallbackIniPath(), result)
	end
end

function switch(t, k)
	return t[k] == "TRUE"
end

function setSwitch(t, k, v)
	t[k] = v and "TRUE" or "FALSE"
end

function param(t, k)
	return t[k] / paramScales[k]
end

function setParam(t, k, v)
	t[k] = math.floor(v * paramScales[k] + 0.5)
end

-- Runner

class "Song"

function Song:Song(spec)
	pull(self,spec)
	pull(self,{})
end

function Song:atiter(at)
	while true do
		at.y = at.y + 1
		if at.y > #self.switch then at.x = at.x + 1 at.y = 1 end
		if self.switch[at.y] then break end
	end
end

function Song:tempomod()
	return self.param[2] > 0 and self.param[2]*16 or 2.25
end

-- Takes ini table
function Song:load(ini)
	self.notes = SparseGrid()
	self.ini = ini
	self.switch = {}
	self.param = {}

	do local any = false
		for i,v in ipairs(switches) do -- Unpack switches
			local on = switch(self.ini, v)
			table.insert(self.switch, on)
			if on then any = true end
		end
		for i,v in ipairs(params) do -- Unpack parameters
			local value = param(self.ini, v)
			table.insert(self.param, value)
		end
		if not any then self.switch[1] = true end
	end
	
	-- Unpack name
	local at = {x=1,y=0}
	for ascii in self.ini.PlayerName:gmatch(".") do
		self:atiter(at)
		local n = string.byte(ascii)
		if n > 32 and n < 127 then
			n = n - 33
			self.notes:set(at.x, at.y, n)
		end 
	end
	self.len = at.x
	
	return self
end

-- Returns ini table
function Song:save()
	local maxx = 1
	local empty = true
	for i,v in ipairs(switches) do self.switch[i] = false end -- Clear switch array
	for k,v in self.notes:iter() do
		if k.x > maxx then maxx = k.x end -- Find maximum value
		self.switch[k.y] = true -- Correct switch array
		empty = false
	end

	local newName = ""
	local at = {x=1,y=0}
	while not empty do
		self:atiter(at)
		if at.x > maxx then break end
		local n = self.notes:get(at.x, at.y)
		if (n and (n<0 or n>(127-33))) then n = nil end
		n = n and n + 33 or 32
		newName = newName .. string.char(n)
	end

	self.ini.PlayerName = newName
	
	for i,v in ipairs(switches) do -- Repack switches
		setSwitch(self.ini, v, self.switch[i])
	end

	for i,v in ipairs(params) do -- Unpack parameters
		setParam(self.ini, v, self.param[i])
	end
	
	return self.ini
end

class "Board"

function Board:Board(spec)
	pull(self,spec)
	pull(self,{width=16, cellwidth=X(60), cellheight=X(54), cellpadding=X(8)})
	self.cols = {}
end

function Board:insert(space)
	self.e = ScreenEntity()
	space.rootEntity:addChild(self.e)
	
	for x=1,self.width do
		local col = {}
		for y=1,#switches do
			local e = ScreenShape(ScreenShape.SHAPE_RECT, self.cellwidth, self.cellheight)
			e:setPositionMode(ScreenEntity.POSITION_TOPLEFT)
			e.colorAffectsChildren = false
			e:setColor(0.25,0.25,0.25,1.0)
			self:setBlockPosition(e,x,y)
			
			local t = ScreenLabel("", X(24), "mono")
			t:setPosition(self.cellwidth/4, self.cellheight/4)
			
			e:addChild(t)
			self.e:addChild(e)
			
			table.insert(col, {bg=e, t=t})
		end
		table.insert(self.cols, col)
	end
	return self
end

function Board:setBlockPosition(e,x,y)
	e:setPosition((x-1)*(self.cellwidth + self.cellpadding)+self.cellpadding/2, (y-1)*(self.cellheight + self.cellpadding)+self.cellpadding/2)
end

local notename = {"A","A#","B","C","C#","D","D#","E","F","F#","G","G#"}

function Board:setNote(x,y,v,note)
	local col = self.cols[x]
	if not col then return end
	if v and note then
		local c1 = a(Color(1,1,1,1))
		c1:setColorHSV((note%12)/12*360,(note+33)/128,(note+33)/128)
		local name = string.format("%d%s", math.floor(note/12), notename[note%12 + 1])
		
		col[y].bg.visible = true
		bridge:setColorObj(col[y].bg, c1)
		col[y].t:setText(name)
	else
		col[y].bg.visible = false
	end
end

class "Controller" (Ent)

function Controller:Controller(spec)
	spec = spec or {}
	spec.pressed = tableMerge({
		cursor_left=self.cursor_move, cursor_right=self.cursor_move, cursor_up=self.cursor_move, cursor_down=self.cursor_move,
		pitch_down=self.pitch_mod, pitch_up=self.pitch_mod,	pitch_waydown=self.pitch_mod, pitch_wayup=self.pitch_mod,
		pitch_clear=self.pitch_clear,
		file_revert=self.file_revert, file_save=self.file_save,
	})
	Ent.Ent(self, spec)
	pull(self, {at = 0, sounds = {}})
	for i=1,#switches do
		table.insert(self.sounds, r(BSound()))
	end
end

function Controller:insert()
	Ent.insert(self)
	
	local e = ScreenShape(ScreenShape.SHAPE_RECT, self.board.cellwidth+self.board.cellpadding, self.board.cellheight*#switches + self.board.cellpadding*(#switches+1))
	e:setColor(1,1,0,1)
	self.space.rootEntity:addChild(e)
	self.scrubber = e
	
	self.board:insert(self.space)
	self:imposeSong()
	
	e = ScreenShape(ScreenShape.SHAPE_RECT, self.board.cellwidth, self.board.cellheight)
	e:setPositionMode(ScreenEntity.POSITION_TOPLEFT)
	e:setColor(1,1,1,0)
	e.strokeEnabled = true
	e:setStrokeWidth(4)
	e:setStrokeColor(0,1,1,1)
	self.space.rootEntity:addChild(e)
	self.cursor = e
	self:set_cursor({x=1,y=1})
	
	e = ScreenLabel("Arrow keys: Move cursor | a/d: Raise/lower note | q/e: Raise/lower octave | Space: Clear note", X(12), "mono")
	e:setPosition(X(10), X(35) + self.board.cellheight*#switches + self.board.cellpadding*(#switches+1))
	self.space.rootEntity:addChild(e)
	e = ScreenLabel("For more SHARECART 1000 compatible products, please visit SHARECART1000.COM", X(12), "mono")
	e:setPosition(X(10), X(55) + self.board.cellheight*#switches + self.board.cellpadding*(#switches+1))
	self.space.rootEntity:addChild(e)
	
	return self
end

function Controller:set_cursor(pos)
	self.board:setBlockPosition(self.cursor,pos.x,pos.y)
	self.cursorpos = pos
end

pull(km, {halfstep = math.pow(2,1/12)})

function Controller:imposeSong()
	for i=1,self.board.width do self:writeX(i) end
end

function Controller:writeX(x)
	for i,v in pairs(self.song.switch) do
		local note = self.song.notes:get(x,i)
		self.board:setNote(x,i,v,note)
	end
end

function pitchFrom(semitone)
	return math.pow(km.halfstep, semitone)
end

function Controller:onTick()
	local triggernow = false
	local timenow = Services.Core:getTicksFloat()
	if not self.time_accum then
		self.time_accum = 0
		self.lasttime = timenow
		triggernow = true
	end
	local new_time_accum = self.time_accum + (timenow - self.lasttime) * self.song:tempomod()
	
	if triggernow or math.floor(self.time_accum) ~= math.floor(new_time_accum) then
		self.at = self.at + 1
		self.triggertime = timetick
		
		if self.at > self.song.len then self.at = 1 end
		for i=1,#switches do
			local sound = self.sounds[i]
			local note = self.song.notes:get(self.at,i)
			if note and self.song.switch[i] then
				sound:setPitch(55*pitchFrom(note))
				sound:setVolume(1)
				if not sound:isPlaying() then sound:Play() end
			else
				sound:setVolume(0)
--				if sound:isPlaying() then sound:Stop() end
			end
		end
		
		self.scrubber:setPosition((self.at-0.5)*self.board.cellwidth + (self.at-0.5)*self.board.cellpadding, (self.board.cellheight+self.board.cellpadding)*(#switches)/2)
	end
	
	self.time_accum = new_time_accum
	self.lasttime = timenow
end

function Controller:cursor_move(way)
	local pos = self.cursorpos
	local updown = (way == "cursor_up" or way == "cursor_down")
	if updown then
		local newy = pos.y + (way == "cursor_up" and -1 or 1)
		if newy < 1 or newy > #switches then return end
		pos.y = newy
	else
		local newx = pos.x + (way == "cursor_left" and -1 or 1)
		if newx < 1 or newx > 1024 then return end
		pos.x = newx
	end
	self:set_cursor(pos)
end

function Controller:getNote(x,y)
	return self.song.notes:get(x,y)
end

function Controller:setNote(x,y,note)
	local v = self.song.switch[y]
	self.song.notes:set(x,y,note)
	self.board:setNote(x,y,v,note)
	if note and x > self.song.len then self.song.len = x end
end

function Controller:pitch_mod(way)
	local down = (way == "pitch_down" or way == "pitch_waydown")
	local big = (way == "pitch_waydown" or way == "pitch_wayup")
	local x,y = self.cursorpos.x, self.cursorpos.y
	local note = self:getNote(x,y)
	note = (note or 40) + (down and -1 or 1) * (big and 12 or 1)
	self.song.switch[y] = true
	self:setNote(x,y,note)
end

function Controller:pitch_clear()
	local x,y = self.cursorpos.x, self.cursorpos.y
	self:setNote(x,y,nil)
end

function Controller:file_save()
	saveIni(self.song:save(), defaultIniPath())
end

function Controller:file_revert()
--	self.song:load(defaultIni())
--	self:imposeSong()

	-- Incorrect but at this point I barely care.
	want_rebirth = true
end

-- For UI
function registerPopupEnt(window, key)
    local ke = Ent({listener = Queue(), windowHidden = true, onInput = function (self)
        local is = false
        while true do
            local entry = self.listener:pop()
            if not entry then break end
            if entry[1] == key then is = true end -- p key to bring up
        end
        if is and self.windowHidden then window:showWindow() self.windowHidden = false end
    end})
    window:addEventListener(nil, function()
        --window:hideWindow()
        --ke.windowHidden = true
    end, UIEvent.CLOSE_EVENT)
    table.insert(listeners, ke.listener)
    ke:insert()
end