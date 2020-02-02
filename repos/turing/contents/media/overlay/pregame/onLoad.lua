-- Pregame -- On Load

pull(km, {default={steps=12,repeats=1, noisesteps=0,noiserepeats=1,resolution=1,loopsize=256,symbolcount=5,statecount=5}, iparams={"steps","repeats","noisesteps","noiserepeats","resolution"}, uniparams={}, params={}, unparams={}, recreate_after=30 })

for i,v in ipairs(km.iparams) do km.uniparams[v] = i end
for i,v in ipairs(km.params) do km.unparams[v] = i end

-- Util

local uiscale = surface_height/600
function X(x)
	return x * uiscale
end

-- ARCHITECTURE:
-- Machine: On-disk model
-- BSound: Audio-playback model
-- Controller: Glue code quarantine
-- Frontend: User-facing UI / "view"

class "Machine"

function Machine:Machine(spec)
	pull(self,km.default)
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
    print(self.code)
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

class "Controller" (Ent)

function Controller:Controller(spec)
	spec = spec or {}
	spec.pressed = tableMerge({
	}, spec.pressed)
	Ent.Ent(self, spec)
	pull(self, {recreation = 1})
end

function Controller:insert()
	Ent.insert(self)
	
    self:recreate()
	
	return self
end

function Controller:uploadSymbols()
    if self.machine.symbolmap then
        self.sound:setSymbols( self.machine.symbolmap )
    end
end

function Controller:recreate()
    self.want_recreate = nil
    cdelete(self.sound)
    
    self.recreation = self.recreation + 1
    self.sound = r(BSound(self.machine.loopsize, self.machine.symbolcount, self.machine.statecount))
    self.sound:upload(self.machine.code)
    self:uploadSymbols()
    
    for i,v in ipairs(km.iparams) do
        self.sound:setIparam(i-1, self.machine[v])
    end
    
    for i,v in ipairs(km.params) do
        self.sound:setParam(i-1, self.machine[v])
    end
    
    self.sound:Play()
end

function Controller:loopScramble()
	self.sound:loopScramble()
end

function Controller:unstick()
    local current = self.sound:current()
    local state, symbol = unpack(current)
    local newcode = self.machine:onerandom()
    self.machine.code[state][symbol] = newcode
    self.sound:adjust(state, symbol, newcode)
end

function Controller:setLoopsize(_loopsize)
    self.machine.loopsize = _loopsize
    self.machine.symbolmap = nil
    self.want_recreate = ticks
end

function Controller:setSymbolcount(_symbolcount)
    self.machine.symbolcount = _symbolcount
    self.machine:randomize()
    self.want_recreate = ticks
end

function Controller:setStatecount(_symbolcount)
    self.machine.statecount = _symbolcount
    self.machine:randomize()
    self.want_recreate = ticks
end

function Controller:iparam(which, value)
    local i = km.uniparams[which]
    if i then
        self.machine[which] = value
        self.sound:setIparam(i-1, value)
    end
end

function Controller:onTick()
    if self.want_recreate and ticks-self.want_recreate>km.recreate_after then
        self.want_recreate = nil
        self:recreate()
    end
end

class "Frontend" (Ent)

function Frontend:Frontend(spec)
    spec = spec or {}
    Ent.Ent(self,spec)
    pull(self, {})
    self.space = self.space or r(Screen())
end

function Frontend:insert()
    Ent.insert(self)

    self.oscWidth = surface_width/2
    self.oscHeight = surface_height/2
    self.osc = ScreenShape(ScreenShape.SHAPE_RECT, self.oscWidth, self.oscHeight)
    self.osc:setPosition(surface_width/2 * 2/3, surface_height/2)
    self.space.rootEntity:addChild(self.osc)
    
    -- YANK THIS
--	e = ScreenLabel("Arrow keys: Move cursor | a/d: Raise/lower note | q/e: Raise/lower octave | Space: Clear note | u: Revert | o: Save", X(12), "mono")
--	e:setPosition(X(10), X(35))
--	self.space.rootEntity:addChild(e)

    return self
end

function Frontend:onTick()
    if self.last_recreation ~= self.controller.recreation then
        self.last_recreation = self.controller.recreation
        self.controller.sound:initTexture(self.oscWidth, self.oscHeight)
    end
    
    local texture = self.controller.sound:oscTexture()
    if texture then
        cdelete( self.osc:getTexture() )
        self.osc:setTexture( texture )
    end
end

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
        window:hideWindow()
        ke.windowHidden = true
    end, UIEvent.CLOSE_EVENT)
    table.insert(listeners, ke.listener)
    ke:insert()
end