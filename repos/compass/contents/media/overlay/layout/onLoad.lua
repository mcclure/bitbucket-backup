-- Layout manager on load

-- TODO: r() IS NOT WHAT I WANT

class "Layout"

function Layout:Layout(spec)
	pull(self,spec)
end

function Layout:onSlider(ctx, event) -- Appropriate?
	local sender = UIHSlider(UIElement(ScreenEntity(Entity(event:getDispatcher()))))
	ctx.act(ctx.target, sender:getSliderValue())
end

function Layout:onText(ctx, event) -- Appropriate?
	local sender = UITextInput(UIElement(ScreenEntity(Entity(event:getDispatcher()))))
	local value = sender:getText()
	local isint = ctx.valuekind == "int"
	if ctx.valuekind == "Number" or isint then 
		value = tonumber(value)
		if value and isint then
			value = math.floor(value)
		end
	end
	ctx.act(ctx.target, value)
end

function Layout:onButton(ctx, event) -- Appropriate?
	ctx.act(ctx.target, ctx.tag)
end

function Layout:onCheck(ctx, event) -- Appropriate?
	local sender = UICheckBox(UIElement(ScreenEntity(Entity(event:getDispatcher()))))
	ctx.act(ctx.target, sender:isChecked())
end

function Layout:push(t, proto, asif)
	if not proto then proto = {} end
	pull(proto,t.proto) pull(proto,t.aproto)
	asif = asif or t.kind
	
	if asif == "fold" then -- Collection, not an item
		for i,entry in ipairs(t) do
			self:push(entry, proto)
		end
	elseif asif == "check" then -- Checkboxes have no label
		local input = r(UICheckBox(t[1], t.default or false))
		table.insert(self.inp, {input, ystill=t.ystill, yoff=4} )
		function onCheck(ctx,event) ctx[1]:onCheck(ctx, event) end
		input:addEventListener( {self, target=t.target, act=t.act, valuekind=t.valuekind}, onCheck, UIEvent.CHANGE_EVENT)
	elseif asif == "trigger" then -- Button
		local input = r(UIButton(t[1], 45, 20))
		table.insert(self.inp, {input, xoff=146, yoff=4} ) -- NO NO NO!! On the xoff
		function onButton(ctx,event) ctx[1]:onButton(ctx, event) end
		input:addEventListener( {self, target=t.target, act=t.act, tag=t.tag}, onButton, UIEvent.CLICK_EVENT)
	else -- Some item with a label
		t = tableMerge(proto,t)
		
		if t[1] then table.insert(self.inp, {r(ScreenLabel(t[1], 15))}) end
		
		if asif == "slider" then
			local slider = r(UIHSlider(t.min, t.max, 160))
			slider:setContinuous(true)
			table.insert(self.inp, {slider, xoff=20, ystill=t.ystill} )
			function onSlider(ctx,event) ctx[1]:onSlider(ctx, event) end
			local numdefault = t.default and tonumber(t.default) if numdefault then slider:setSliderValue(numdefault) end
			slider:addEventListener( {self, target=t.target, act=t.act}, onSlider, UIEvent.CHANGE_EVENT)
		elseif asif == "text" then
			local input = r(UITextInput(false, 100, 15))
			table.insert(self.inp, {input, xoff=13, yafter=t.ystill and 0 or 5, ystill=t.ystill} )
			function onText(ctx,event) ctx[1]:onText(ctx, event) end
			if t.default then input:setText(to_string(t.default)) end
			input:addEventListener( {self, target=t.target, act=t.act, valuekind=t.valuekind}, onText, UIEvent.COMPLETE_EVENT)
		elseif asif == "sliderex" then
			local basexoff = 8
			local slidwid = 120
			local slider = r(UIHSlider(t.min, t.max, slidwid))
			slider:setContinuous(true)
			local input = r(UITextInput(false, 40, 12))
			
			local default = t.default and tonumber(t.default)
			if t.default then input:setText(to_string(t.default)) slider:setSliderValue(t.default) end
			
			table.insert(self.inp, {slider, xoff=basexoff, ystill=true} )
			function onSlider(ctx,event)
				ctx[1]:onSlider(ctx, event)
				
				function mate(obj, value) obj:setText(string.format("%0.2f", value)) end
				ctx[1]:onSlider( {self, target=ctx.partner, act=mate}, event )
			end
			slider:addEventListener( {self, target=t.target, act=t.act, partner=input}, onSlider, UIEvent.CHANGE_EVENT)

			table.insert(self.inp, {input, xoff=basexoff+slidwid+12, yoff=t.ystill and 0 or -8, ystill=t.ystill} )
			function onText(ctx,event)
				ctx[1]:onText(ctx, event)
				
				function mate(obj, value) obj:setSliderValue(value) end
				ctx[1]:onText( {self, target=ctx.partner, act=mate, valuekind="Number"}, event )
			end
			
			input:addEventListener( {self, target=t.target, act=t.act, valuekind="Number", partner=slider}, onText, UIEvent.COMPLETE_EVENT)
		end
	end
	
	if t.instant then t.act(t.target, t.default) end
end

function Layout:build()
	local ydown = 40
	for i,elem in ipairs(self.inp) do
		elem[1].position.x = 20 + (elem.xoff or 0)
		elem[1].position.y = ydown + (elem.yoff or 0)
		self.target:addChild(elem[1])
		if not elem.ystill then ydown = ydown + 25 end
		if elem.yafter then ydown = ydown + elem.yafter end
	end
end

function Layout:insert()
	self.inp = {}
	
	self:push(self.contents, nil, "fold")
	self:build()
	
	self.contents = nil	
	self.inp = nil
	
	return self
end