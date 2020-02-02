-- On Load

killDos(false, km.gfxw, km.gfxh)
dos = type_automaton()
gfx = dos:toGfx()
dos:insert()

dos:set_centered(0,4,40,"SUPER FUNGUS ATTACK!!")
dos:set_centered(0,5,40,"BY RUN HELLO")

dos:set_centered(0,11,40,"PLAYER 1 YOU ARE  RED")
dos:set_centered(0,12,40,"PLAYER 2 YOU ARE BLUE")

dos:set_centered(0,19,40,"BEGIN? Y/N ")
