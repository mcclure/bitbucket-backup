if au.alone then au.alone:Stop() end
killdos()
if not dos then
	dos = type_automaton()
	dos:insert()
end
dos:set_centered(0,11,40,"END OF AVAILABLE CONTENT")
glitter_update(dos, "cohere")