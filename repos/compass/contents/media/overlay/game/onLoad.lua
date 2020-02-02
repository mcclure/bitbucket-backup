-- Globals

if _DEBUG then -- Preserve room across esc
	if remember_at then at = remember_at end
	function sav() remember_at = at end
end

-- Setup DOS, shaders

killDos()
dos = type_automaton()
dos:insert()
-- dos:displayScene():getDefaultCamera():setPostFilter("FilterMaterial")

-- do
--	shadname = {"x3", "x2", "x1", "x0", "y3", "y2", "y1", "y0"}
--	local shadspec = {} for i,name in ipairs(shadname) do shadspec[name] = 0 end
--	shader = shaderBindings(dos:displayScene():getDefaultCamera(), shadspec)
-- end

-- Begin

sanity_check(km.map)

gm.typer = Typer()
gm.roomer = InputRoomer({typer=gm.typer, map=km.map, start=(_DEBUG and fm.lastAt)})

gm.typer:insert()
gm.typer:print("Second Person\n\nBy Andi\n\nPlay with these keys: \008 \011 \012 \021\n\nPress any to start\n\n")

gm.roomer:insert()