if au.alone then au.alone:Stop() end
killdos()
if not dos then
	dos = type_automaton()
	dos:insert()
end
dos:set(2,4,"As the noxious gas fills your lungs,")
dos:set(2,5,"your limbs grow heavy. Your sight")
dos:set(2,6,"dims, and you feel yourself falling")
dos:set(2,7,"to the floor.")
dos:set_centered(0,19,40,"You are dead.")
glitter_update(dos, "cohere")