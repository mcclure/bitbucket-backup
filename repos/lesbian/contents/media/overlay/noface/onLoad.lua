-- On Load
ticks = 0

killDos()
dos = type_automaton()
dos:insert()

dos:set_centered(0,11,40,string.format("CASTLE %d COMPLETE!",fm.level))