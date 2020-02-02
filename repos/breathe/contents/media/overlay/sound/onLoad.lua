-- Music load
if not bgmusic then
	bgmusic = Sound("media/untitled.ogg")
end

function reset_sound(_slipat, _slipstart, _slipend, _playstart, _playend, _soft)
	if not bgmusic:isPlaying() then bgmusic:Play(true) end
	slipat = _slipat slipstart = _slipstart slipend = _slipend
	playstart = _playstart playend = _playend
	if _playstart and not _soft then
		bgmusic:setOffset(playstart)
		lastslip = playstart
	end
end