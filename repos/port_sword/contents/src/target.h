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

#ifndef __target_h__
#define __target_h__

#include "mapdata.h"
#include "glbdef.h"

class CIVSTATE;

class TARGET
{
public:
    TARGET();
    TARGET(float value, MAPDATA distmap, TILE_NAMES tile, int x, int y);
    TARGET(const TARGET &target);
    TARGET &operator=(const TARGET &target);
    ~TARGET();

    float		getValue() const { return myValue; }
    TILE_NAMES		getTile() const { return myTile; }
    MAPDATA		getDistMap() const { return myDistMap; }
    bool		isCity() const { return myTile == TILE_CITY_RED || myTile == TILE_CITY_VIOLET; }
    int			x() const { return myX; }
    int			y() const { return myY; }

    int			owner(CIVSTATE *state) const;

protected:
    float		myValue;
    TILE_NAMES		myTile;
    int			myX, myY;
    MAPDATA		myDistMap;
};

#endif
