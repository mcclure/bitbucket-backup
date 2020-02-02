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


#include <libtcod.hpp>
#include "display.h"
#include "scrpos.h"
#include "gfxengine.h"
#include "mob.h"
#include "item.h"

DISPLAY::DISPLAY(int x, int y, int w, int h)
{
    myX = x;
    myY = y;
    myW = w;
    myH = h;
    myMapId = -1;

    myMemory = new u8[myW * myH];
    myMemoryPos = new POS[myW * myH];
    myYellFlags = new u8[myW * myH];
    myFadeFromWhite = false;
    myPosCache = 0;
    clear();
}

DISPLAY::~DISPLAY()
{
    delete myPosCache;
    delete [] myMemory;
    delete [] myMemoryPos;
    delete [] myYellFlags;
}

void
DISPLAY::fadeFromWhite()
{
    myFadeFromWhite = true;
    myWhiteFadeTS = TCOD_sys_elapsed_milli();
}

void
DISPLAY::clear()
{
    memset(myMemory, ' ', myW * myH);
    for (int i = 0; i < myW * myH; i++)
	myMemoryPos[i] = POS();
    delete myPosCache;

    myPosCache = 0;
}

POS
DISPLAY::lookup(int x, int y) const
{
    POS		p;

    x -= myX + myW/2;
    y -= myY + myH/2;

    if (myPosCache)
	p = myPosCache->lookup(x, y);

    return p;
}

void
DISPLAY::scrollMemory(int dx, int dy)
{
    u8		*newmem = new u8[myW * myH];
    POS		*newpos = new POS[myW * myH];
    int		x, y;

    memset(newmem, ' ', myW * myH);

    for (y = 0; y < myH; y++)
    {
	if (y + dy < 0 || y + dy >= myH)
	    continue;
	for (x = 0; x < myW; x++)
	{
	    if (x + dx < 0 || x + dx >= myW)
		continue;

	    newmem[x+dx + (y+dy)*myW] = myMemory[x + y*myW];
	    newpos[x+dx + (y+dy)*myW] = myMemoryPos[x + y*myW];
	}
    }
    delete [] myMemory;
    myMemory = newmem;
    delete [] myMemoryPos;
    myMemoryPos = newpos;
}

void
DISPLAY::rotateMemory(int angle)
{
    angle = angle & 3;
    if (!angle)
	return;

    u8		*newmem = new u8[myW * myH];
    POS		*newpos = new POS[myW * myH];
    int		x, y;
    int		offx, offy;

    offx = myW/2;
    offy = myH/2;

    memset(newmem, ' ', myW * myH);

    for (y = 0; y < myH; y++)
    {
	for (x = 0; x < myW; x++)
	{
	    switch (angle)
	    {
		case 1:
		    newmem[x + y*myW] = recall(y-offy+offx, myW - x-offx+offy);
		    newpos[x + y*myW] = recallPos(y-offy+offx, myW - x-offx+offy);
		    break;

		case 2:
		    newmem[x + y*myW] = recall(myW - x, myH - y);
		    newpos[x + y*myW] = recallPos(myW - x, myH - y);
		    break;

		case 3:
		    newmem[x + y*myW] = recall(myH - y-offy+offx, x-offx+offy);
		    newpos[x + y*myW] = recallPos(myH - y-offy+offx, x-offx+offy);
		    break;

	    }
	}
    }
    delete [] myMemory;
    myMemory = newmem;
    delete [] myMemoryPos;
    myMemoryPos = newpos;
}

u8
DISPLAY::recall(int x, int y) const
{
    if (x < 0 || x >= myW) return ' ';
    if (y < 0 || y >= myH) return ' ';
    return myMemory[x + y * myW];
}

POS
DISPLAY::recallPos(int x, int y) const
{
    if (x < 0 || x >= myW) return POS();
    if (y < 0 || y >= myH) return POS();
    return myMemoryPos[x + y * myW];
}

void
DISPLAY::note(int x, int y, u8 val, POS pos)
{
    if (x < 0 || x >= myW) return;
    if (y < 0 || y >= myH) return;

    myMemory[x + y * myW] = val;
    myMemoryPos[x + y * myW] = pos;
}

void
DISPLAY::drawYellMessage(int px, int py, int dx, int dy,
			const char *text,
			ATTR_NAMES attr)
{
    int		textlen = strlen(text);

    YELL_STATE	flag;

    flag = yellFlag(px+dx, py+dy);

    // We will draw the slash over undrawn space.
    // we can also draw it immediately beside text without ambiguity.
    if (flag == YELLSTATE_NONE || flag == YELLSTATE_PAD)
    {
	gfx_printchar( px+dx + x(), py+dy + y(),
			// The following line deserves extra commenting.
			// This text is that extra commenting.
		       ((dx+1)^(dy+1)) ? '/' : '\\',
		       attr );
	// Note what is there.
	setYellFlag(px+dx, py+dy, YELLSTATE_SLASH);
    }

    // Get proposed center of text
    px += dx * 2;
    py += dy * 2;

    // Now search for fitting room.
    // We allow it to migrate in any direction matching our dy.
    findTextSpace(px, py, dy, textlen);

    // We always succeed (but px/py may be out of bounds, but we don't
    // care, because print char will ignore those)
    for (int i = 0; i < textlen; i++)
    {
	gfx_printchar( px - textlen/2 + i + x(),
			py + y(),
			text[i],
			attr );
	setYellFlag(px - textlen/2 + i, py, YELLSTATE_TEXT);
    }
    // Add our pads.
    setYellFlag(px - textlen/2 - 1, py, YELLSTATE_PAD);
    setYellFlag(px - textlen/2 + textlen, py, YELLSTATE_PAD);
}

void
DISPLAY::setYellFlag(int x, int y, YELL_STATE state)
{
    if (x < 0 || y < 0)
	return;

    if (x >= width() || y >= height())
	return;

    myYellFlags[x + y * width()] = state;
}

DISPLAY::YELL_STATE
DISPLAY::yellFlag(int x, int y) const
{
    if (x < 0 || y < 0)
	return YELLSTATE_NONE;

    if (x >= width() || y >= height())
	return YELLSTATE_NONE;

    return (YELL_STATE) myYellFlags[x + y * width()];
}

bool
DISPLAY::checkRoomForText(int px, int py, int textlen)
{
    // Make room for padding
    textlen += 2;

    // Always fit off screen.
    if (py < 0 || py >= height())
	return true;

    YELL_STATE		flag;

    for (int i = 0; i < textlen; i++)
    {
	flag = yellFlag(px - textlen/2 + i, py);

	if (flag == YELLSTATE_PAD || flag == YELLSTATE_TEXT)
	    return false;
    }

    // We can write over everything else
    return true;

}

void
DISPLAY::findTextSpace(int &px, int &py, int dy, int textlen)
{
    int			r, ir;

    for (r = 0; r < width()+height(); r++)
    {
	// Check vertical shift
	for (ir = 0; ir <= r; ir++)
	{
	    if (checkRoomForText(px+ir, py+dy*r, textlen))
	    {
		px += ir;
		py += dy*r;
		return;
	    }
	    if (checkRoomForText(px-ir, py+dy*r, textlen))
	    {
		px -= ir;
		py += dy*r;
		return;
	    }
	}

	// Check horizontal shift
	for (ir = 0; ir <= r; ir++)
	{
	    if (checkRoomForText(px+r, py+ir, textlen))
	    {
		px += r;
		py += ir;
		return;
	    }
	    if (checkRoomForText(px-r, py+ir, textlen))
	    {
		px -= r;
		py += ir;
		return;
	    }
	}
    }

    assert(!"We should always hit the auto-none areas and succeed");
    return;
}


void
DISPLAY::display(POS pos, bool isblind)
{
    int		dx, dy;
    int		x, y;

    if (!pos.map() || pos.map()->getId() != myMapId)
    {
	clear();
	if (pos.map())
	    myMapId = pos.map()->getId();
	else
	    myMapId = -1;
    }

    if (myPosCache && myPosCache->find(pos, dx, dy))
    {
	POS		oldpos = myPosCache->lookup(dx, dy);

	// Make pos the center
	scrollMemory(-dx, -dy);
	// Now rotate by it.
	rotateMemory(oldpos.angle() - pos.angle());
    }
    else
    {
	// We made too big a jump, clear!
	clear();
    }

    delete myPosCache;
    myPosCache = new SCRPOS(pos, (myW+1)/2, (myH+1)/2);

    // Clear all memorized spots that don't line up with visible spots
    for (y = 0; y < myH; y++)
    {
	for (x = 0; x < myW; x++)
	{
	    pos = myPosCache->lookup(x-myW/2, y-myH/2);
	    if (!pos.valid() || !pos.isFOV() || (isblind && !(x == myW/2 && y == myH/2)))
	    {
		continue;
	    }

	    // This spot is visible
	    FORALL_8DIR(dx, dy)
	    {
		// Make sure the neighbours still match what we
		// recorded.
		if (pos.delta(0, dy).delta(dx, 0) != recallPos(x+dx, y+dy)
			&&
		    pos.delta(dx, 0).delta(0, dy) != recallPos(x+dx, y+dy))
		{
		    note(x+dx, y+dy, ' ', POS());
		}
	    }
	}
    }

    for (y = 0; y < myH; y++)
    {
	for (x = 0; x < myW; x++)
	{
	    pos = myPosCache->lookup(x-myW/2, y-myH/2);
	    if (!pos.valid() || !pos.isFOV() || (isblind && !(x == myW/2 && y == myH/2)))
	    {
		gfx_printchar(x+myX, y+myY, recall(x, y), ATTR_OUTOFFOV);
	    }
	    else
	    {
		u8		symbol;
		ATTR_NAMES	attr;

		note(x, y, pos.defn().symbol, pos);
		if (pos.mob())
		{
		    pos.mob()->getLook(symbol, attr);
		    gfx_printchar(x+myX, y+myY, symbol, attr);
		}
		else if (pos.item())
		{
		    pos.item()->getLook(symbol, attr);
		    gfx_printchar(x+myX, y+myY, symbol, attr);
		    note(x, y, pos.item()->defn().symbol, pos);
		}
		else
		{
		    if (pos.defn().roomcolor)
			gfx_printchar(x+myX, y+myY, pos.defn().symbol, 
				pos.color()[0], pos.color()[1], pos.color()[2]);
		    else
			gfx_printchar(x+myX, y+myY, pos.defn().symbol, 
					(ATTR_NAMES) pos.defn().attr);
		}
	    }
	}
    }

    // Apply events...
    int		timems = TCOD_sys_elapsed_milli();

    // First process new events.
    EVENT	e;

    while (queue().remove(e))
    {
	e.setTime(timems);
	myEvents.append(e);
    }

    int		i;

    // The events list is in chronological order.  This means
    // new events draw over old events.
    // Yell messages, however, are weird, because they need to be
    // drawn in the opposite order so the newest shows up nearest
    // the yeller and pushes out old yells.
    // Thus two loops.
    // MOB lookup, and culling too old events, are handled
    // in the first loop.
    for (i = 0; i < myEvents.entries(); i++)
    {
	e = myEvents(i);

	int		len = 150;
	if (e.type() & EVENTTYPE_LONG)
	    len = 2000;

	if (timems - e.timestamp() > len)
	{
	    myEvents.removeAt(i);
	    i--;
	    continue;
	}

	if (e.mobuid() != INVALID_UID)
	{
	    MOB		*mob;
	    mob = pos.map()->findMob(e.mobuid());
	    if (!mob)
	    {
		myEvents.removeAt(i);
		i--;
		continue;
	    }
	    e.setPos(mob->pos());
	    myEvents(i) = e;
	}

	if (!myPosCache->find(e.pos(), x, y))
	{
	    // No longer on screen.
	    // Do not remove, it may come back on screen
	    // for long events!
	    continue;
	}

	// Get the actual pos which we can use for FOV
	pos = myPosCache->lookup(x, y);
	if (pos.isFOV())
	{
	    int		px = x + myX + myW/2;
	    int		py = y + myY + myH/2;

	    // Draw the event.
	    if (e.type() & EVENTTYPE_SHOUT)
	    {
		// Ignore shouts in this pass.
	    }
	    else
	    {
		// Just a symbol
		if (e.type() & EVENTTYPE_FORE)
		    gfx_printattrfore(px, py, e.attr());
		if (e.type() & EVENTTYPE_BACK)
		    gfx_printattrback(px, py, e.attr());
		if (e.type() & EVENTTYPE_SYM)
		    gfx_printchar(px, py, e.sym());
	    }
	}
    }

    // Backwards loop for yells.
    // Clear our yell state
    memset(myYellFlags, YELLSTATE_NONE, width()*height());
    for (i = myEvents.entries(); i --> 0; )
    {
	e = myEvents(i);

	if (!(e.type() & EVENTTYPE_SHOUT))
	    continue;

	if (!myPosCache->find(e.pos(), x, y))
	{
	    // No longer on screen.
	    // Do not remove, it may come back on screen
	    // for long events!
	    continue;
	}

	// Get the actual pos which we can use for FOV
	pos = myPosCache->lookup(x, y);
	if (pos.isFOV())
	{
	    int		px = x + myW/2;
	    int		py = y + myH/2;

	    // Draw the shout text.
	    if (e.text())
	    {
		int		dx = SIGN(x);
		int		dy = SIGN(y);

		if (!dx)
		    dx = 1;

		if (!dy)
		    dy = -1;

		drawYellMessage(px, py, dx, dy, e.text(), e.attr());
	    }
	}
    }
}

void
DISPLAY::redrawFade()
{
    if (!myFadeFromWhite)
	return;

    float		fade;

    fade = 0.0f;
    if (myFadeFromWhite)
    {
	int timems = TCOD_sys_elapsed_milli();
	fade = (timems - myWhiteFadeTS) / 3000.;
	if (fade > 1.0f)
	{
	    myFadeFromWhite = false;
	    fade = 1.0f;
	}
    }

    for (int y = 0; y < SCR_HEIGHT; y++)
    {
	for (int x = 0; x < SCR_WIDTH; x++)
	{
	    gfx_fadefromwhite(x, y, fade);
	}
    }
}
