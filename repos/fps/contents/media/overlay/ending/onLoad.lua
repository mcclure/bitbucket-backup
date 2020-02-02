-- Game over screen setup

if m_ambient then m_ambient:Stop() end

dos = type_automaton()
dos:insert()

dos_write = {
	{ 60*0, 6,  "CONGRATULATIONS.", },
	{ 60*1, 10, "YOU~HAVE~DONE~WHAT~NO~HUMAN", },
	{ 60*2, 11, "HAS~EVER~DONE~BEFORE:", },
	{ 60*3, 12, "YOU~HAVE~SAVED~MARS." },
	{ 60*4, 18, "PRESS~ESC", },
}