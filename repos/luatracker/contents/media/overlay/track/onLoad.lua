-- Tracker

tk = {halfstep = math.pow(2,1/12)}
tk.pitchFrom = function(semitone) return math.pow(tk.halfstep, semitone) end

-- Utility

class "MiniTimer"

function MiniTimer:MiniTimer(cycle,modes)
	self.cycle = cycle
	self.modes = modes
	self.start = Services.Core:getTicksFloat()
end

function MiniTimer:get()
	local now = Services.Core:getTicksFloat()
	local since = now - self.start
	if self.modes then
		local count = math.floor(since/self.cycle)
		if self.modes > 0 then count = count % self.modes end
		return count
	else
		if since > self.cycle then
			self.start = now - ( now % self.cycle )
			return true
		else
			return false
		end
	end
end

-- 3 separate concepts: "source", "voice", "channel".
-- channels at some point hypothetically should be decomposed into nouns and verbs
-- PROVISIONALLY, "VOICE" IS ALWAYS 1
-- PROVISIONALLY, FREESTYLE NOT SUPPORTED

-- Track machine-- set tm.immortal to survive across room changes
-- Tracker spec:
--  (bool) halted (bool) patternlock (table key->at-int) patternat
-- Song spec:
--	(table) patterns: List of pattern objects (patterns key) start
-- Pattern spec:
--	(int) div: "time signature", number of items. 0 == "freestyle"
--  (int) len: length of pattern in measures.
--  (table) notes:
--		FOR FREESTYLE: array not dict, values are {at 0-1, {voice, channel, value} }
--		FOR NORMAL:	dict time => {voice, channel, value}
-- Channels:
--  (int) pp -- merge of play and pitch channels. 'false' for stop.

class "Tracker"

function Tracker:Tracker(spec, song)
	pull(self, {bpm=120, song={}, patternat = {}})
	pull(self, spec)
	if song then
		pull(self.song, song)
	else
		pull(self.song, {patterns={{}}, start=1})
	end
	
	if self.span then
		self:setSpan(self.span)
	else
		self:setBpm(self.bpm)
	end
	
	self:reset()
end

function Tracker:setSpan(span)
	self.bpm = 1/span/60
	if span ~= self.span then
		self.span = span
		self:adjustTimer()
	end
end
function Tracker:setBpm(bpm)
	self.span = 60/bpm
	if bpm ~= self.bpm then
		self.bpm = bpm
		self:adjustTimer()
	end
end

function Tracker:adjustTimer() -- TODO
end

function Tracker:reset(unpause)
	self.state = self.song.start
	self.patternat[self.state] = 0
	self.timer = MiniTimer(1/self.span)
	if unpause then self.paused = false end
	self:start(self.paused)
end

function Tracker:start(pause) -- FIXME is argument stupid?
	self.paused = pause
	if play then self:step(0) end
end
function Tracker:step(by)
	for k,v in pairs(self.patternat) do
		local pattern = self.song.patterns[k]
		if by>0 then
			v = (v + by) % pattern.div
			self.patternat[k] = v
		end
		local act = pattern.notes[v]
		if act then
			for channel, value in pairs(act) do
				if channel == "pp" then
					if value then
						if not au.tone:isPlaying() then au.tone:Play(true) end
						au.tone:setPitch(tk.pitchFrom(value))
					else
						if au.tone:isPlaying() then au.tone:Stop() end
					end
				end
			end
		end
	end
end

function Tracker:tick()
	if self.paused then return end
	if self.timer:get() then
		self:step(1)
	end
end

function Tracker:die()
end