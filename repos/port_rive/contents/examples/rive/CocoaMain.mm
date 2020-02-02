
#include "Game.h"

#include <Cocoa/Cocoa.h>
#include <unistd.h>

//---------------------------------------------------------------------
int main(int argc, char **argv)
{
	NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
	NSRect usableRect = [NSWindow contentRectForFrameRect:[[NSScreen mainScreen] visibleFrame]
								styleMask:NSTitledWindowMask|NSClosableWindowMask|NSResizableWindowMask];
	chdir([resourcePath UTF8String]);
	ld::windowSize = usableRect.size.height;

	// init
	Game game;

	// run
	game.run();

	// end
	return 0;
}
