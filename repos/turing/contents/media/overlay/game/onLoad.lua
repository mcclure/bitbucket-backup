-- On Load

pull(gm, {r=Router():insert()})

-- Set up "spotlight" effect
--screen():setScreenShader("FilterMaterial")
--shader = shaderBindings(screen(), 
--	{center=a(Vector2(0.1,0.1)), radius=0.1, intensity=100.0, aspect=(surface_width/surface_height)})


--print( defaultIni() )

--sound = r(BSound())
--sound:Play(true)

if nextmachine then
	print(nextmachine)
	gm.machine = Machine(nextmachine)
	nextmachine = nil
else
	gm.machine = Machine():randomize()
end
gm.controller = Controller({machine=gm.machine}):insert()
gm.frontend = Frontend({controller=gm.controller}):insert()

adhocui = true

if adhocui then

uiscreen = r(Screen())

if false then -- No mouse needed
uinet = r(ScreenEntity())
uinet.processInputEvents = true
uinet.visible = false
--uinet:setPositionMode(ScreenEntity.POSITION_TOPLEFT)
uinet:setPosition(surface_width/2,surface_height/2)
uiscreen:addChild(uinet)
uinet:setHitbox(surface_width,surface_height)
uinet:addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEMOVE)
uinet:addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEDOWN)
uinet:addEventListener(input, input.handleEvent, InputEvent.EVENT_MOUSEUP)
end

end
