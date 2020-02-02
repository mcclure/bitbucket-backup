
#include "Game.h"

// Windows main()
//---------------------------------------------------------------------
int __stdcall WinMain(__in int hInstance, __in int hPrevInstance, __in const char *lpCmdLine, __in int nCmdShow)
{
	// init
	Game game;

	// run
	game.run();

	// end
	return 0;
}
