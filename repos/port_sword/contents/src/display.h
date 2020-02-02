/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        display.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 * 	Draws a view into a map.  Handles timed effects, map memory, etc.
 */

#ifndef __display__
#define __display__

#include "map.h"

#include "event.h"

class SCRPOS;

class DISPLAY
{
public:
    enum YELL_STATE
    {
	YELLSTATE_NONE,
	YELLSTATE_SLASH,
	YELLSTATE_TEXT,		// Contains speach
	YELLSTATE_PAD,		// 1 char padding around speach
    };

    // Rectangle inside of TCOD to display.
    DISPLAY(int x, int y, int w, int h);
    ~DISPLAY();

    void	 display(POS pos, bool isblind=false);
    // This is separate from the display as you have to do it
    // as separate past post-drawing other overlays.
    void	 redrawFade();

    // This is in absolute screen coords, ie, bakes in myX and myY
    POS		 lookup(int x, int y) const;

    // Erases our memory.
    void	 clear();

    int		 width() const { return myW; }
    int		 height() const { return myH; }
    int		 x() const { return myX; }
    int		 y() const { return myY; }

    EVENT_QUEUE	 &queue() { return myEQ; }

    void	 fadeFromWhite();

    void	 postHealthChange(int change) { myHealthQueue.append(change); }
    bool	 popHealthChange(int &change) { return myHealthQueue.remove(change); }

private:
    void	 scrollMemory(int dx, int dy);
    void	 rotateMemory(int angle);
    u8		 recall(int x, int y) const;
    POS		 recallPos(int x, int y) const;
    void	 note(int x, int y, u8 val, POS pos);

    void	 setYellFlag(int x, int y, YELL_STATE state);
    YELL_STATE	 yellFlag(int x, int y) const;

    void	 drawYellMessage(int px, int py, int dx, int dy,
				const char *text,
				ATTR_NAMES attr);

    bool	 checkRoomForText(int px, int py, int textlen);
    void	 findTextSpace(int &px, int &py, int dy, int textlen);

    u8		*myMemory;
    POS		*myMemoryPos;
    u8		*myYellFlags;
    int		 myX, myY;
    int		 myW, myH;
    int		 myMapId;
    SCRPOS	*myPosCache;

    int		 myWhiteFadeTS;
    bool	 myFadeFromWhite;

    // Where we track all of our active events.
    EVENTLIST	 myEvents;

    // Where incoming events go
    EVENT_QUEUE	 myEQ;
    QUEUE<int>	 myHealthQueue;
};

#endif
