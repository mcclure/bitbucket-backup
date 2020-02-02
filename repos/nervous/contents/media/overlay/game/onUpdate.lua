-- On Update

want = {x=0,y=0}

if pressed[KEY_ESCAPE] then
	bridge:load_room_txt("media/outro.txt")
elseif pressed[KEY_LEFT] then
	want.x = want.x - km.dxy
elseif pressed[KEY_RIGHT] then
	want.x = want.x + km.dxy
elseif pressed[KEY_UP] then
	want.y = want.y - km.dxy
elseif pressed[KEY_DOWN] then
	want.y = want.y + km.dxy
end

do
	player:try(want.x,"x")
	player:try(want.y,"y")
end

if basstimer:get() then
	bass()
end

pressed = {}