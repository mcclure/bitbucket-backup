/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Sword In Hand Development
 *
 * NAME:        target.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 * 	A stragetic target
 */

#include "target.h"
#include "civstate.h"

TARGET::TARGET()
{
    myValue = 0;
}

TARGET::TARGET(float value, MAPDATA distmap, TILE_NAMES tile, int x, int y)
{
    myValue = value;
    myDistMap = distmap;
    myTile = tile;
    myX = x;
    myY = y;
}

TARGET::TARGET(const TARGET &target)
{
    *this = target;
}

TARGET::~TARGET()
{
}

TARGET &
TARGET::operator=(const TARGET &target)
{
    myDistMap = target.myDistMap;
    myValue = target.myValue;
    myTile = target.myTile;
    myX = target.myX;
    myY = target.myY;
    return *this;
}

int
TARGET::owner(CIVSTATE *state) const
{
    int		owner = -1;

    if (state->army(0)(x(), y()) > 0)
    {
	owner = 0;
    }
    else if (state->army(1)(x(), y()) > 0)
    {
	owner = 1;
    }
    else
    {
	// Check the tile.
	if (getTile() == TILE_CITY_RED)
	    owner = 1;
	else if (getTile() == TILE_CITY_VIOLET)
	    owner = 0;
    }
    return owner;
}
