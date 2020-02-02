-- Tracker debug

-- State and constants

-- Far as I'm concerned BPM means Measures Per Minute. Maybe I'll change BPM later.

tdk = {
	shift = function() return tdm.down[KEY_LSHIFT] or tdm.down[KEY_RSHIFT] end,
	ctop=1,ctall=8
}
tdm = {modename="free",octave=0,offset=0, lastinput=""}

tdk.keynames = {"A","A+","B","C","C+","D","D+","E","F","F+","G","G+","A2","A+2","B2"}
tdk.keys = {
	[KEY_a]=0,  -- A
	[KEY_w]=1,  -- A#
	[KEY_s]=2,  -- B
	[KEY_d]=3,  -- C
	[KEY_r]=4,  -- C#
	[KEY_f]=5,  -- D
	[KEY_t]=6,  -- D#
	[KEY_g]=7,  -- E
	[KEY_h]=8,  -- F
	[KEY_u]=9,  -- F#
	[KEY_j]=10, -- G
	[KEY_i]=11, -- G#
	[KEY_k]=12, -- A2
	[KEY_o]=13, -- A#2
	[KEY_l]=14, -- B2
}

if not dos then
	dos = type_automaton()
	dos:insert()
end

function refresh_meta()
	dos:fill(0,0,40,1)
	dos:set(0,0,string.format("mode:%s oct:%d off:%d input:%s", tdm.modename, tdm.octave, tdm.offset, tdm.lastinput))
end

-- UI

class "BaseMode"
function BaseMode:input() end
function BaseMode:tick() end
function BaseMode:enter() end
function BaseMode:exit() end

class "OffMode" (BaseMode)
function OffMode:input()
	if tdm.pressed[KEY_BACKSLASH] then
		setMode(tdm.lastmode)
	end
end

function handleNoteInput()
	local semitone
	
	for k,v in pairs(tdk.keys) do
		if tdm.pressed[k] then
			semitone = v
		end
	end
	
	if tdm.pressed[KEY_LEFTBRACKET] then
		tdm.octave = tdm.octave - 1 refresh_meta()
	elseif tdm.pressed[KEY_RIGHTBRACKET] then
		tdm.octave = tdm.octave + 1 refresh_meta()
	elseif tdm.pressed[KEY_QUOTE] then
		tdm.offset = tdm.offset + 1 refresh_meta()
	elseif tdm.pressed[KEY_SEMICOLON] then
		tdm.offset = tdm.offset - 1 refresh_meta()
	elseif tdm.pressed[KEY_SPACE] then
		tdm.lastinput = "X"		refresh_meta()
		return false
	elseif semitone then
		tdm.lastinput = tdk.keynames[semitone+1] refresh_meta()
		-- TODO: What if need the raw value?
		return semitone + tdm.offset + tdm.octave*12
	end
end

class "FreeMode" (BaseMode)
function FreeMode:input()
	if tdm.pressed[KEY_BACKSLASH] then
		setMode(tdk.shift() and "pattern" or "off")
	else
		local semitone = handleNoteInput()
		if semitone ~= nil then
			if semitone then 
				local note = tk.pitchFrom(semitone)
				if not au.tone:isPlaying() then
					au.tone:Play(true)
				end
				au.tone:setPitch(note)
			else
				au.tone:Stop()
			end
		end
	end
end
function FreeMode:exit()
	au.tone:Stop()
end

function fillRect(r)
	dos:fill(r[1],r[2],r[3],r[4])
end

class "ScrollingMode" (BaseMode)
function ScrollingMode:exit()
	fillRect(self.frame)
end

class "PatternMode" (ScrollingMode)
function PatternMode:PatternMode()
	pull(self, {frame={0,tdk.ctop,40,tdk.ctall}, cellw=2})
end
function PatternMode:input()
	if tdm.pressed[KEY_F1] then -- SAVE
		bridge:saveTableIntoFile("music.xml", "music", tm.song)
	elseif tdm.pressed[KEY_F2] then -- LOAD
		local song = bridge:loadTableFromFile("music.xml",true)
		if song then
			tm.song = song
		else
			tdm.lastinput = "FAIL" refresh_meta()
		end
	elseif tdm.pressed[KEY_PERIOD] then -- BPM UP
		local bpm = tm.bpm - 1
		tm:setBpm(bpm)
		tdm.lastinput = string.format("%dBPM",bpm) refresh_meta()
		tm:reset()
	elseif tdm.pressed[KEY_SLASH] then -- BPM DOWN
		local bpm = tm.bpm + 1
		tm:setBpm(bpm)
		tdm.lastinput = string.format("%dBPM",bpm) refresh_meta()
		tm:reset()
	elseif tdm.pressed[KEY_BACKSLASH] then
		setMode(tdk.shift() and "song" or "off")
	else
		local semitone = handleNoteInput()
		if semitone ~= nil then
			local k = 1
			local pattern = tm.song.patterns[k]
			local beat = (tm.patternat[k] + 1) % pattern.div
			if not pattern.notes[beat] then pattern.notes[beat] = {} end
			pattern.notes[beat].pp = semitone
		end
	end
end
function PatternMode:enter()
	tm:reset(true)
end
function PatternMode:exit()
	tm:start(true) -- pause
end
function PatternMode:tick()
	dos:fill(self.frame[1],self.frame[2],40,1)
	dos:set(self.frame[1]+tm.patternat[tm.state],self.frame[2],"*")
end

class "SongMode" (ScrollingMode)
function SongMode:SongMode()
	pull(self, {frame={0,ctop,40,ctall}, })
end
function SongMode:input()
	if tdm.pressed[KEY_BACKSLASH] then
		setMode(tdk.shift() and "free" or "off")
	end
end

local default_modes = {off=OffMode(), free=FreeMode(), pattern=PatternMode(), song=SongMode()}
function setMode(name)
	if tdm.mode then tdm.mode:exit() end
	tdm.lastmode = tdm.modename
	tdm.modename = name
	tdm.mode = default_modes[name]
	if tdm.mode then tdm.mode:enter() end
	tdm.lastinput = ""
	refresh_meta()
end
setMode("free")

-- Song 

tm = Tracker({paused = true, bpm=15}, {patterns={{div=32,len=1,notes={}}}, start=1}
)

-- Set up input handler
tdm.down = {} tdm.pressed = {}
class "Input" (EventHandler)
function Input:Input()
	self:EventHandler()
end

function Input:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == EVENT_KEYDOWN then
			tdm.down[key] = true
			tdm.pressed[key] = true
		elseif e:getEventCode() == EVENT_KEYUP then
			tdm.down[key] = nil
		end
	end
end

do
	input = Input()
	
	Services.Core:getInput():addEventListener(input, EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, EVENT_KEYUP)
end