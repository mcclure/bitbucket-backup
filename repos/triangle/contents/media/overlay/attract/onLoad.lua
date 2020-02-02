-- On Load

Services.Renderer:setClearColor(0.0,0.0,0.0,1)
--Services.Core:enableMouse(false)
	
killDos()
dos = type_automaton()
dos:insert()

strings = {
"L?L?L?L",
"\008 \011 \010 \021",
}

maxlen = 0
for i,v in ipairs(strings) do
	local vl = #v
	if vl > maxlen then maxlen = vl end
end

typetop = (24 - (#strings*2-1))/3
typeleft = (40 - maxlen)/2

for i,v in ipairs(strings) do
	local top = i == #strings
	if top then
		dos:set(typeleft,typetop + i*3,v)
	else
		dos:set(typeleft,typetop + i*3 - 1,v)
	end
end

if nil then for i=1,24 do -- Grid
	dos:set(30,i-1,string.format("%d",i))
end end