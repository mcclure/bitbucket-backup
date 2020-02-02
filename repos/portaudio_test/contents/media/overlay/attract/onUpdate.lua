-- On Update

if pressed[KEY_1] or pressed[KEY_2] then
	if pressed[KEY_2] then 
		fm.coeff = 2
		fm.invert = true
	end
	nextroom("game")
elseif pressed[KEY_ESCAPE] then
	bridge:Quit()
end

pressed = {}