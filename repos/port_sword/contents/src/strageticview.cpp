/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Sword In Hand Development
 *
 * NAME:        strategy.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 * 	Display of stragetic map of overview.
 */

#include "gfxengine.h"
#include "map.h"
#include "scrpos.h"
#include "strategy.h"
#include "civstate.h"
#include "strageticview.h"
#include "target.h"

STRAGETICVIEW::STRAGETICVIEW(int x, int y, int w, int h)
{
    myX = x;
    myY = y;
    myW = w;
    myH = h;
    myCurrentYear = 0;

    myStrategy = 0;
    myScrPos = 0;
}

STRAGETICVIEW::~STRAGETICVIEW()
{
    if (myStrategy)
	myStrategy->decRef();
}


void
STRAGETICVIEW::setStrategy(STRATEGY *strategy)
{
    if (myStrategy)
	myStrategy->decRef();
    myStrategy = strategy;
    POS		center = strategy->getCenter();

    delete myScrPos;
    myScrPos = new SCRPOS(center, width()/2, height()/2);
}

void
rampFromAttribute(int owner, float value, float scale, int &r, int &g, int &b)
{
    if (value <= 0 || owner < 0)
    {
	r = g = b = 0;
	return;
    }

    value /= scale;
    if (value > 1.0) value = 1.0;

    if (owner == 0)
    {
	r = value * 128 + 127;
	g = value * 96;
	b = value * 128 + 127;
    }
    else
    {
	r = value * 128+127;
	g = value * 96;
	b = value * 96;
    }
}

void
STRAGETICVIEW::rewind()
{
    myCurrentYear = 0;
}

void
STRAGETICVIEW::playFrame(int inc)
{
    myCurrentYear += inc;
    if (myStrategy)
    {
	if (myCurrentYear > myStrategy->getMaxYear())
	    myCurrentYear = 0;
	if (myCurrentYear < 0)
	    myCurrentYear = myStrategy->getMaxYear();
    }
}

void
STRAGETICVIEW::display()
{
    CIVSTATE		*today = getToday();
    for	(int y = 0; y < height(); y++)
    {
	for (int x = 0; x < width(); x++)
	{
	    int		scrx = x + myX;
	    int		scry = y + myY;

	    if (myScrPos && myStrategy && today)
	    {
		POS		p = myScrPos->lookup(x - width()/2, y - height()/2);

		int		owner = -1;
		float		army = 0;
		if (today->army(0)(x, y) > 0)
		{
		    owner = 0;
		    army = today->army(0)(x, y);
		}
		if (today->army(1)(x, y) > 0)
		{
		    owner = 1;
		    army = today->army(1)(x, y);
		}

		// Invalid positions are transparent.
		// Could make a jagged map this way, I guess :>
		if (p.valid())
		{
		    int		br, bg, bb;
		    float	dist;

		    rampFromAttribute(owner, army, 255, br, bg, bb);
		    gfx_printchar(scrx, scry, 
			    p.defn().symbol,
			    glb_attrdefs[p.defn().attr].fg_r,
			    glb_attrdefs[p.defn().attr].fg_g,
			    glb_attrdefs[p.defn().attr].fg_b,

			    br, bg, bb);
		}
	    }
	    else
	    {
		// All blank, likely shouldn't happen...
		gfx_printchar(scrx, scry,
			      ' ',
			      ATTR_NORMAL);
	    }
	}
    }
}
