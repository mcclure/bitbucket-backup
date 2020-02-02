/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Sword In Hand Development
 *
 * NAME:        strageticview.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 * 	Display of stragetic map of overview.
 */

#ifndef __strageticview__
#define __strageticview__

#include "strategy.h"

class SCRPOS;
class STRATEGY;
class CIVSTATE;

// Apparently I don't know hwo to spell strategic.
class STRAGETICVIEW
{
public:
    // Rectangle inside of TCOD to display.
    STRAGETICVIEW(int x, int y, int w, int h);
    ~STRAGETICVIEW();

    void		display();

    int		 width() const { return myW; }
    int		 height() const { return myH; }

    // Gain ownership, strategy should already be incRefed!
    void	 setStrategy(STRATEGY *strategy);

    STRATEGY	*getStrategy() const { return myStrategy; }

    CIVSTATE	*getToday() const { return myStrategy ? myStrategy->getYear(myCurrentYear) : 0; }
    int		 getCityCount(int civ) const { return (myStrategy&&getToday()) ? myStrategy->cityCount(civ, getToday()) : 0; }

    int		 getYear() const { return myCurrentYear; }

    // Play controls...
    void	 playFrame(int inc);
    void	 rewind();

private:
    int		 myX, myY;
    int		 myW, myH;

    int		 myCurrentYear;

    STRATEGY	*myStrategy;
    SCRPOS	*myScrPos;
};

#endif
