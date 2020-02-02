for k,v in pairs(au) do
	if type(v) == "table" and v.Stop then v:Stop() end
end