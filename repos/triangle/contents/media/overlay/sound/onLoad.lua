-- Pregame -- On Load

skm = {}
pull(skm, {default={steps=12,repeats=1, noisesteps=0,noiserepeats=1,resolution=1,loopsize=256,symbolcount=5,statecount=5}, iparams={"steps","repeats","noisesteps","noiserepeats","resolution"}, uniparams={}, params={}, unparams={}, recreate_after=30 })

for i,v in ipairs(skm.iparams) do skm.uniparams[v] = i end
for i,v in ipairs(skm.params) do skm.unparams[v] = i end

-- Util

-- ARCHITECTURE:
-- Machine: On-disk model
-- BSound: Audio-playback model
-- SndController: Glue code quarantine

class "Machine"

function Machine:Machine(spec)
	pull(self,skm.default)
	pull(self,spec)
end

function Machine:randomize()
    self.code = {}
    self.symbolmap = nil
    for state=1,self.statecount do
        local statecode = {} table.insert(self.code, statecode)
        for symbol=1,self.symbolcount do
            local symbolcode = self:onerandom()
            table.insert(statecode, symbolcode)
        end
    end
    return self
end

function Machine:statescramble()
    self.symbolmap = {}
    for i = 1,self.symbolcount do
        table.insert(self.symbolmap, math.random()*2-1)
    end
end

function Machine:onerandom()
    return {math.random(self.statecount), math.random(self.symbolcount), math.random(-1,1)}
end

class "SndController" (Ent)

function SndController:SndController(spec)
	spec = spec or {}
	spec.pressed = tableMerge({
	}, spec.pressed)
	Ent.Ent(self, spec)
	pull(self, {recreation = 1})
end

function SndController:insert()
	Ent.insert(self)
	
    self:recreate()
	
	return self
end

function SndController:uploadSymbols()
    if self.machine.symbolmap then
        self.sound:setSymbols( self.machine.symbolmap )
    end
end

function SndController:recreate()
    self.want_recreate = nil
    cdelete(self.sound)
    self.recreation = self.recreation + 1
    self.sound = r(BSound(self.machine.loopsize, self.machine.symbolcount, self.machine.statecount))
    self.sound:upload(self.machine.code)
    self:uploadSymbols()
    self:loopScramble()
	
    for i,v in ipairs(skm.iparams) do
        self.sound:setIparam(i-1, self.machine[v])
    end
    
    for i,v in ipairs(skm.params) do
        self.sound:setParam(i-1, self.machine[v])
    end
    
    self.sound:Play()
end

function SndController:loopScramble()
	self.sound:loopScramble()
	return self
end

function SndController:unstick()
    local current = self.sound:current()
    local state, symbol = unpack(current)
    local newcode = self.machine:onerandom()
    self.machine.code[state][symbol] = newcode
    self.sound:adjust(state, symbol, newcode)
end

function SndController:setLoopsize(_loopsize)
    self.machine.loopsize = _loopsize
    self.machine.symbolmap = nil
    self.want_recreate = ticks
end

function SndController:setSymbolcount(_symbolcount)
    self.machine.symbolcount = _symbolcount
    self.machine:randomize()
    self.want_recreate = ticks
end

function SndController:setStatecount(_symbolcount)
    self.machine.statecount = _symbolcount
    self.machine:randomize()
    self.want_recreate = ticks
end

function SndController:iparam(which, value)
    local i = skm.uniparams[which]
    if i then
        self.machine[which] = value
        self.sound:setIparam(i-1, value)
    end
end

function SndController:disabled_onTick()
    if self.want_recreate and ticks-self.want_recreate>skm.recreate_after then
        self.want_recreate = nil
        self:recreate()
    end
end