-- UI Test -- common -- on load

if km.adhocui then

uiscreen = r(Screen())

local ke = Ent({listener = Queue(), windowVisible=false, kids={},
	flip = function(self, to)
		Services.Core:enableMouse(to)
		if to == self.windowVisible then return end
		self.windowVisible = to
		gm.freezeInput = self.windowVisible
		if gm.freezeInput then gm.lastMouseAt = nil end
		if to then
			for i,v in ipairs(self.kids) do v:showWindow() end
		else
			for i,v in ipairs(self.kids) do v:hideWindow() end
		end
	end,
	addKid = function(self, window)
		table.insert(self.kids, window)
		window:addEventListener(nil, function()
			self:flip(false)
		end, UIEvent.CLOSE_EVENT)
		if not self.windowVisible then window:hideWindow() end
	end,
	onInput = function (self)
		if pressed[KEY_z] then self:flip(true) end
	end
}):insert()

function setupWindow(window)
	ke:addKid(window)
end

end