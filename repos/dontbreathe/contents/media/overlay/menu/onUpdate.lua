if pressed[KEY_ESCAPE] then
	bridge:Quit()
elseif needing then
	local material = color and "color" or "grayscale"
	if needing == 1 then
		bridge:load_room(
			"media/overlay/startup\n" ..
			string.format("media/ext_material/%s.mat\n", material) ..
			"media/overlay/game\n" ..
			"media/overlay/sound\n" ..
			"media/overlay/intro\n" ..
			"media/bg.svg\n" ..
			"media/example.svg\n" ..
			"media/overlay/shutdown"
		)
	else
		bridge:load_room(
			"media/overlay/startup\n" ..
			string.format("media/ext_material/%s.mat\n", material) ..
			"media/overlay/game\n" ..
			"media/overlay/sunset\n" ..
			"media/bg.svg\n" ..
			"media/example2.svg\n" ..
			"media/overlay/shutdown"
		)
	end
else
	if pressed[KEY_2] then
		needing = 1
		special_reverse = false
		rule30 = false
	elseif pressed[KEY_1] then
		needing = 1
		special_reverse = true
		rule30 = false
	elseif pressed[KEY_3] then
		needing = 2
		special_reverse = true
		rule30 = true
	end
	
	if pressed[KEY_g] then
		color = false
		print(color)
	elseif pressed[KEY_c] then
		color = true
		print(color)
	end
	
	if needing and dos then
		dos:set_centered(0,19,40,"",true,true)
		dos:set_centered(0,20,40,"PLEASE HOLD",true,true)
		dos:set_centered(0,21,40,"",true,true)
	end
end

pressed = {}