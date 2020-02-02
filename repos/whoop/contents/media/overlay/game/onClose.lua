for i,v in ipairs(gm.w) do
	v.tone:Stop()
	delete(v.tone)
end
gm.w = {}

killDos()