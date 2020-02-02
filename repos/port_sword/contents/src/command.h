/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        command.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 *	The command queue processing all the game commands
 *	into the game engine.
 */

#ifndef __command__
#define __command__

#include "glbdef.h"
#include "queue.h"

class COMMAND
{
public:
    COMMAND()
    {
	myAction = ACTION_NONE;
    }
    explicit COMMAND(int blah)
    {
	myAction = ACTION_NONE;
    }
    COMMAND(ACTION_NAMES action, int dx = 0, int dy = 0, int dz = 0)
    {
	myAction = action;
	myDx = dx;
	myDy = dy;
	myDz = dz;
    }

    ACTION_NAMES	action() const { return myAction; }
    int			dx() const { return myDx; }
    int			dy() const { return myDy; }
    int			dz() const { return myDz; }

private:
    ACTION_NAMES		myAction;
    int				myDx;
    int				myDy;
    int				myDz;
};

typedef QUEUE<COMMAND> CMD_QUEUE;

#endif
