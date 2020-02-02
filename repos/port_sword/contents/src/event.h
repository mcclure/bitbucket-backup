/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        event.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 *	The event queue sends display info back to the display.
 */

#ifndef __event__
#define __event__

#include "glbdef.h"
#include "queue.h"
#include "mob.h"
#include "item.h"

class EVENT
{
public:
    EVENT()
    {
	mySym = '!';
	myAttr = ATTR_PURPLE;
	myFirstTS = -1;
	myType = EVENTTYPE_NONE;
	myText = 0;
	myMobUID = INVALID_UID;
    }
    explicit EVENT(int blah)
    {
	mySym = '!';
	myAttr = ATTR_PURPLE;
	myFirstTS = -1;
	myType = EVENTTYPE_NONE;
	myText = 0;
	myMobUID = INVALID_UID;
    }
    EVENT(POS p, u8 sym, ATTR_NAMES a, EVENTTYPE_NAMES type, const char *text=0)
    {
	myPos = p;
	mySym = sym;
	myAttr = a;
	myFirstTS = -1;
	myType = type;
	myText = text;
	myMobUID = INVALID_UID;
    }
    EVENT(MOB *m, u8 sym, ATTR_NAMES a, EVENTTYPE_NAMES type, const char *text=0)
    {
	myPos = POS();
	mySym = sym;
	myAttr = a;
	myFirstTS = -1;
	myType = type;
	myText = text;
	myMobUID = m->getUID();
    }

    POS		pos() const { return myPos; }
    u8		sym() const { return mySym; }
    ATTR_NAMES	attr() const { return myAttr; }
    const char *text() const { return myText; }
    int		timestamp() const { return myFirstTS; }
    void	setTime(int ts) { myFirstTS = ts; }
    EVENTTYPE_NAMES type() const { return myType; }
    int		mobuid() const { return myMobUID; }

    void	setPos(POS p) { myPos = p; }

private:
    u8		mySym;
    ATTR_NAMES	myAttr;
    POS		myPos;
    int		myMobUID;
    int		myFirstTS;	// Timestamp we proced.
    EVENTTYPE_NAMES	myType;
    const char	*myText;
};

typedef QUEUE<EVENT> EVENT_QUEUE;
typedef PTRLIST<EVENT> EVENTLIST;

#endif

