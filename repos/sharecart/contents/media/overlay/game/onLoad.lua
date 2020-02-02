-- On Load

pull(gm, {r=Router():insert()})

-- Set up "spotlight" effect
--screen():setScreenShader("FilterMaterial")
--shader = shaderBindings(screen(), 
--	{center=a(Vector2(0.1,0.1)), radius=0.1, intensity=100.0, aspect=(surface_width/surface_height)})


--print( defaultIni() )

--sound = r(BSound())
--sound:Play(true)

gm.controller = Controller({ song=Song():load(defaultIni()), board=Board(), space=screen()}):insert()