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

#ifndef __scrpos__
#define __scrpos__

#include "map.h"

class SCRPOS
{
public:
    // VAlid for -radw..radw and -radh..radh inclusive.
    SCRPOS(POS center, int radw, int radh);
    ~SCRPOS();

    POS		lookup(int x, int y) const;

    bool	find(POS pos, int &x, int &y) const;

private:
    void	set(int x, int y, POS pos);

    POS		*myPos;
    int		 myRadW, myRadH;
};

#endif

