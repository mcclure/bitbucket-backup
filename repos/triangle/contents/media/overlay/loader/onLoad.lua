-- Room loader utils - on load

_want_rebirth = nil
_want_room = nil
if _DEBUG or not _room_template then
	_room_template = bridge:filedump("media/game.txt")
end

function nextroom(room)
	if room then
		_want_room = room
	else
		_want_rebirth = room
	end
end

function nextlevel(room)
	if room then
		room = string.format(_room_template, room)
	end
	nextroom(room)
end