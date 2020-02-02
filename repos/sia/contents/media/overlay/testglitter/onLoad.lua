bridge:coreServices().drawScreensFirst = false

if dos then dos:die() end
if makefree then
	dos = freetype_automaton()
else
	dos = type_automaton()
end
dos:insert()

dos:load_text("media/testdos.obj")

if not makereadable then
	glitter_update(dos, "basic")
end