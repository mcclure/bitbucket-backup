-- On Load

pull(gm, {r=Router():insert(), })

gm.a = Player({is="a", base="one", notebase=0}):insert()
gm.b = Player({is="b", base="two", notebase=3, pressed={}}):insert()
gm.ref = Ref():connect(gm.a,gm.b):insert()

local ai = {freq=64, lofreq=16, player=gm.b, ref=gm.ref,
	pushplus=function(self, t, func)
		self.schedule[self:ticks() + t] = func
		print(schedule)
	end,
	underly=function(self)
		if self.ref.state == "tryagain" then self:die() return end
		
		local move = string.format("b_%d", math.random(3))
		self.player:move(move)
	end,
	trigger=function(self)
		self:underly()
		self:pushplus(self.freq, self.trigger)
		for i=1,math.random(-2,2) do
			self:pushplus(i*self.lofreq, self.trigger)
		end
	end,
}
ai.schedule={[ai.freq]=ai.trigger}
Clock(ai):insert()