-- On Update

local refresh = (ticks % km.on == 0)

if refresh then
	dos:clear()
	dos:set_centered(0,4,40,"Devil's Chord")
	dos:set_centered(0,12,40,"Some buttons that do things:")
	dos:set_centered(0,15,40,"Click")
	dos:set_centered(0,16,40,"Right-click (or shift-click)")
	dos:set_centered(0,17,40,"Control-click")
	dos:set_centered(0,18,40,"Left and right arrow keys")
	dos:set_centered(0,19,40,"Number keys")
	dos:set_centered(0,20,40,"R and B")
end

for i,v in ipairs(gm.w) do
	v:tick(refresh)
end

if pressed[0] or pressed[1] or pressed[KEY_SPACE] or pressed[KEY_RETURN] then
	nextroom("phase")
end

if pressed[KEY_RIGHT] then km.aon = km.aon - (km.aon > 1 and 1 or 0) end
if pressed[KEY_LEFT] then km.aon = km.aon + 1 end

if pressed[KEY_ESCAPE] then
	bridge:Quit()
end

pressed = {}