/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        scrpos.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 *	Constructs a screen-pos array from a given position.
 *	Allows fast lookup of POS given a x/y index in world pos.
 */

#include "scrpos.h"

SCRPOS::SCRPOS(POS center, int radw, int radh)
{
    myPos = new POS [(radw*2+1) * (radh*2+1)];
    myRadW = radw;
    myRadH = radh;

    // Now build it...
    int				r, i;

    set(0, 0, center);

    for (r = 1; r <= MAX(myRadW, myRadH); r++)
    {
	for (i = -r; i <= r; i++)
	{
	    set(r, i, lookup(r-1, i - SIGN(i)).delta(1, SIGN(i)));
	    set(-r, i, lookup(-(r-1), i - SIGN(i)).delta(-1, SIGN(i)));
	    set(i, r, lookup(i - SIGN(i), r-1).delta(SIGN(i), 1));
	    set(i, -r, lookup(i - SIGN(i), -(r-1)).delta(SIGN(i), -1));
	}
    }
}

SCRPOS::~SCRPOS()
{
    delete [] myPos;
}

POS
SCRPOS::lookup(int x, int y) const
{
    if (x < -myRadW || x > myRadW)
	return POS();
    if (y < -myRadH || y > myRadH)
	return POS();
    return myPos[x + myRadW + (y + myRadH) * (myRadW * 2 + 1)];
}

void
SCRPOS::set(int x, int y, POS pos)
{
    if (x < -myRadW || x > myRadW)
	return;
    if (y < -myRadH || y > myRadH)
	return;

    myPos[x + myRadW + (y + myRadH) * (myRadW * 2 + 1)] = pos;
}

bool
SCRPOS::find(POS pos, int &x, int &y) const
{
    // Do a spiral search, we want to find the closest.
    int			r, i;

    for (r = 0; r <= MAX(myRadH, myRadW); r++)
    {
	for (i = -r; i <= r; i++)
	{
	    if (lookup(r, i) == pos)
	    {
		x = r;		y = i;
		return true;
	    }
	    if (lookup(-r, i) == pos)
	    {
		x = -r;		y = i;
		return true;
	    }
	    if (lookup(i, r) == pos)
	    {
		x = i;		y = r;
		return true;
	    }
	    if (lookup(i, -r) == pos)
	    {
		x = i;		y = -r;
		return true;
	    }
	}
    }
    return false;
}
