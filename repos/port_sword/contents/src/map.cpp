/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        map.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 */

#include <libtcod.hpp>

#include "map.h"

#include "mob.h"
#include "item.h"

#include "text.h"

#include "dircontrol.h"
#include "scrpos.h"
#include "display.h"
#include "msg.h"

#include <fstream>
using namespace std;

// #define DO_TIMING

#define ORIENT_FLIPX	1
#define ORIENT_FLIPY	2
#define ORIENT_FLOP	4

PTRLIST<class FRAGMENT *> MAP::theFragRooms[NUM_ROOMTYPES];

bool	glbRoomDiscovered[NUM_ROOMTYPES];
bool	glbRoomLooted[NUM_ROOMTYPES];
int	glbManaSpent[NUM_SPELLS];
QUEST_NAMES	glbCurrentQuest;
QUEST_NAMES	glbLastQuest;
WORLDSTATE_NAMES	glbWorldState;
int	glbLevel = 1;
int	glbMobsInFOV[NUM_MOBS];

class FRAGMENT
{
public:
    FRAGMENT(const char *fname);
    ~FRAGMENT();

    ROOM		*buildRoom(MAP *map, ROOMTYPE_NAMES type) const;

protected:
    void		 swizzlePos(int &x, int &y, int orient) const;

    int		myW, myH;
    u8		*myTiles;
    BUF		myFName;
    friend class ROOM;
};

POS::POS()
{
    myRoomId = -1;
    myMap = 0;
    myX = myY = myAngle = 0;
}

POS::POS(int blah)
{
    myRoomId = -1;
    myMap = 0;
    myX = myY = myAngle = 0;
}

bool	
POS::valid() const 
{ 
    if (tile() == TILE_INVALID) return false; 
    return true; 
}

TILE_NAMES	
POS::tile() const 
{ 
    if (!room()) return TILE_INVALID; 
    TILE_NAMES		tile = room()->getTile(myX, myY); 

    return tile;
}

void	
POS::setTile(TILE_NAMES tile) const
{ 
    if (!room()) return; 
    return room()->setTile(myX, myY, tile); 
}

MAPFLAG_NAMES 		
POS::flag() const 
{ 
    if (!room()) return MAPFLAG_NONE; 
    return room()->getFlag(myX, myY); 
}

void	
POS::setFlag(MAPFLAG_NAMES flag, bool state) const
{ 
    if (!room()) return; 
    return room()->setFlag(myX, myY, flag, state); 
}

POS
POS::spiralSearch(int startrad, bool allowmob) const
{
    if (!room()) return POS();
    return room()->spiralSearch(*this, startrad, allowmob);
}

bool
POS::prepSquareForDestruction() const
{
    ROOM		*r = room();
    int			dx, dy;

    if (!valid())
	return false;

    if (!r)
	return false;

    if (!myX || !myY || myX == r->width()-1 || myY == r->height()-1)
    {
	// Border tile, borders to nothing, so forbid dig
	return false;
    }

    FORALL_8DIR(dx, dy)
    {
	if (r->getTile(myX+dx, myY+dy) == TILE_INVALID)
	{
	    // Expand our walls.
	    r->setTile(myX+dx, myY+dy, TILE_WALL);
	}
	// Do not dig around portals!
	if (r->getFlag(myX+dx, myY+dy) & MAPFLAG_PORTAL)
	    return false;
    }

    // If this square is a portal, don't dig!
    if (r->getFlag(myX, myY) & MAPFLAG_PORTAL)
	return false;

    return true;
}

bool
POS::digSquare() const
{
    if (!defn().isdiggable)
	return false;

    if (!prepSquareForDestruction())
	return false;

    TILE_NAMES		nt;

    nt = TILE_FLOOR;
    if (tile() == TILE_WALL)
	nt = TILE_BROKENWALL;

    setTile(nt);

    return true;
}

ROOMTYPE_NAMES
POS::roomType() const
{
    if (!room()) return ROOMTYPE_NONE;
    return room()->type();
}

MOB *
POS::mob() const 
{ 
    if (!room()) return 0; 
    return room()->getMob(myX, myY); 
}

int
POS::getAllMobs(MOBLIST &list) const
{
    if (!room()) return list.entries();

    room()->getAllMobs(myX, myY, list);
    return list.entries();
}

int
POS::getDistance(int distmap) const 
{ 
    if (!room()) return -1; 

    return room()->getDistance(distmap, myX, myY); 
}

void
POS::setDistance(int distmap, int dist) const 
{ 
    if (!room()) return; 

    return room()->setDistance(distmap, myX, myY, dist); 
}

int
POS::getAllItems(ITEMLIST &list) const
{
    if (!room()) return list.entries();

    room()->getAllItems(myX, myY, list);
    return list.entries();
}

int
POS::getAllRoomItems(ITEMLIST &list) const
{
    if (!room()) return list.entries();

    list.append(room()->myItems);
    return list.entries();
}

int
POS::getAllRoomMobs(MOBLIST &list) const
{
    if (!room()) return list.entries();

    list.append(room()->myMobs);
    return list.entries();
}

int
POS::getAllConnectedRoomMobs(MOBLIST &list) const
{
    if (!room()) return list.entries();

    list.append(room()->myMobs);

    ROOMLIST		connected;
    room()->getConnectedRooms(connected);
    for (int i = 0; i < connected.entries(); i++)
    {
	if (connected(i) != room())
	    list.append(connected(i)->myMobs);
    }
    return list.entries();
}

ITEM *
POS::item() const 
{ 
    if (!room()) return 0; 
    return room()->getItem(myX, myY); 
}

int
POS::allItems(ITEMLIST &items) const 
{ 
    items.clear();
    if (!room()) return 0; 
    room()->getAllItems(myX, myY, items);
    return items.entries();
}

ROOM *
POS::room() const 
{ 
    if (myRoomId < 0) return 0; 
    return myMap->myRooms(myRoomId); 
}

u8 *
POS::color() const
{
    static u8 invalid[3] = { 255, 255, 255 };
    if (myRoomId < 0) return invalid;

    return myMap->myRooms(myRoomId)->myColor;
}

int *
POS::jacobian() const
{
    static int j[4] = { 0, 0, 0, 0 };
    int		*newj;
    if (myRoomId < 0) return j;

    newj = myMap->myRooms(myRoomId)->myJacob;
    memcpy(j, newj, sizeof(int) * 4);
    rotateToWorld(j[0], j[2]);
    rotateToWorld(j[1], j[3]);

    return j;
}

void
POS::removeMob(MOB *mob) const
{
    if (myRoomId < 0) return; 

    myMap->myRooms(myRoomId)->removeMob(mob);
}

void
POS::addMob(MOB *mob) const
{
    if (myRoomId < 0) return; 

    myMap->myRooms(myRoomId)->addMob(myX, myY, mob);
}

void
POS::removeItem(ITEM *item) const
{
    if (myRoomId < 0) return; 

    myMap->myRooms(myRoomId)->removeItem(item);
}

void
POS::addItem(ITEM *item) const
{
    if (myRoomId < 0) return; 

    myMap->myRooms(myRoomId)->addItem(myX, myY, item);
}

void
POS::save(ostream &os) const
{
    os.write((const char *)&myX, sizeof(int));
    os.write((const char *)&myY, sizeof(int));
    os.write((const char *)&myAngle, sizeof(int));
    os.write((const char *)&myRoomId, sizeof(int));
}

void
POS::load(istream &is)
{
    is.read((char *)&myX, sizeof(int));
    is.read((char *)&myY, sizeof(int));
    is.read((char *)&myAngle, sizeof(int));
    is.read((char *)&myRoomId, sizeof(int));
}

MOB *
POS::traceBullet(int range, int dx, int dy, int *rangeleft) const
{
    if (rangeleft)
	*rangeleft = 0;
    if (!dx && !dy)
	return mob();

    POS		next = *this;

    while (range > 0)
    {
	range--;
	next = next.delta(dx, dy);
	if (next.mob())
	{
	    if (rangeleft)
		*rangeleft = range;
	    return next.mob();
	}

	// Stop at a wall.
	if (!next.defn().ispassable)
	    return 0;
    }
    return 0;
}

POS
POS::traceBulletPos(int range, int dx, int dy, bool stopbeforewall, bool stopatmob) const
{
    if (!dx && !dy)
	return *this;

    POS		next = *this;
    POS		last = *this;

    while (range > 0)
    {
	range--;
	next = next.delta(dx, dy);

	// Stop at a mob.
	if (stopatmob && next.mob())
	    return next;

	if (!next.defn().ispassable)
	{
	    // Hit a wall.  Either return next or last.
	    if (stopbeforewall)
		return last;
	    return next;
	}
	last = next;
    }
    return next;
}

void
POS::postEvent(EVENTTYPE_NAMES type, u8 sym, ATTR_NAMES attr) const
{
    map()->myDisplay->queue().append(EVENT(*this, sym, attr, type));
}

void
POS::postEvent(EVENTTYPE_NAMES type, u8 sym, ATTR_NAMES attr, const char *text) const
{
    map()->myDisplay->queue().append(EVENT(*this, sym, attr, type, text));
}

void
POS::displayBullet(int range, int dx, int dy, u8 sym, ATTR_NAMES attr, bool stopatmob) const
{
    if (!dx && !dy)
	return;

    POS		next = delta(dx, dy);

    while (range > 0)
    {
	if (!next.valid())
	    return;
	
	// Stop at walls.
	if (!next.isPassable())
	    return;

	// Stop at mobs
	if (stopatmob && next.mob())
	    return;

	next.postEvent(EVENTTYPE_FORESYM, sym, attr);

	range--;
	next = next.delta(dx, dy);
    }
}

void
POS::rotateToLocal(int &dx, int &dy) const
{
    int		odx = dx;
    int		ody = dy;
    switch (myAngle & 3)
    {
	case 0:			// ^
	    break;		
	case 1:			// ->
	    dx = -ody;
	    dy = odx;
	    break;
	case 2:			// V
	    dx = -odx;
	    dy = -ody;
	    break;
	case 3:			// <-
	    dx = ody;
	    dy = -odx;
	    break;
    }
}

void
POS::rotateToWorld(int &dx, int &dy) const
{
    int		odx = dx;
    int		ody = dy;
    switch (myAngle & 3)
    {
	case 0:			// ^
	    break;		
	case 1:			// ->
	    dx = ody;
	    dy = -odx;
	    break;
	case 2:			// V
	    dx = -odx;
	    dy = -ody;
	    break;
	case 3:			// <-
	    dx = -ody;
	    dy = odx;
	    break;
    }
}

int
POS::dist(POS goal) const
{
    if (!goal.valid() || !valid())
    {
	return room()->width() + room()->height();
    }
    if (goal.myRoomId == myRoomId)
    {
	return MAX(ABS(goal.myX-myX), ABS(goal.myY-myY));
    }

    // Check to see if we are in the map cache.
    int		dx, dy;
    if (map()->computeDelta(*this, goal, dx, dy))
    {
	// Yah, something useful.
	return MAX(ABS(dx), ABS(dy));
    }

    // Consider it infinitely far.  Very dangerous as monsters
    // will get stuck on portals :>
    return room()->width() + room()->height();
}

void
POS::dirTo(POS goal, int &dx, int &dy) const
{
    if (!goal.valid() || !valid())
    {
	// Arbitrary...
	dx = 0;
	dy = 1;
	rotateToWorld(dx, dy);
    }

    if (goal.myRoomId == myRoomId)
    {
	dx = SIGN(goal.myX - myX);
	dy = SIGN(goal.myY - myY);

	// Convert to world!
	rotateToWorld(dx, dy);
	return;
    }

    // Check to see if we are in the map cache.
    if (map()->computeDelta(*this, goal, dx, dy))
    {
	// Map is nice enough to give it in world coords.
	dx = SIGN(dx);
	dy = SIGN(dy);
	return;
    }

    // Umm.. Be arbitrary?
    rand_direction(dx, dy);
}

void
POS::setAngle(int angle)
{
    myAngle = angle & 3;
}

POS
POS::rotate(int angle) const
{
    POS		p;

    p = *this;
    p.myAngle += angle;
    p.myAngle &= 3;

    return p;
}

POS
POS::delta4Direction(int angle) const
{
    int		dx, dy;

    rand_getdirection(angle, dx, dy);

    return delta(dx, dy);
}

POS
POS::delta(int dx, int dy) const
{
    if (!valid())
	return *this;

    if (!dx && !dy)
	return *this;

    // We want to make a single step in +x or +y.
    // The problem is diagonals.  We want to step in both xy and yx orders
    // and take whichever is valid.  In most cases they will be equal...
    if (dx && dy)
    {
	POS	xfirst, yfirst;
	int	sdx = SIGN(dx);
	int	sdy = SIGN(dy);


	xfirst = delta(sdx, 0).delta(0, sdy);
	yfirst = delta(0, sdy).delta(sdx, 0);

	if (!xfirst.valid())
	{
	    // Must take yfirst
	    return yfirst.delta(dx - sdx, dy - sdy);
	}
	if (!yfirst.valid())
	{
	    // Must take xfirst.
	    return xfirst.delta(dx - sdx, dy - sdy);
	}

	// Both are valid.  In all likeliehoods, identical.
	// But if one is wall and one is not, we want the non-wall!
	if (!glb_tiledefs[xfirst.tile()].ispassable)
	    return yfirst.delta(dx - sdx, dy - sdy);

	// WLOG, use xfirst now.
	return xfirst.delta(dx - sdx, dy - sdy);
    }

    // We now have a simple case of a horizontal step
    rotateToLocal(dx, dy);

    int		sdx = SIGN(dx);
    int		sdy = SIGN(dy);

    PORTAL	*portal;

    portal = room()->getPortal(myX + sdx, myY + sdy);
    if (portal)
    {
	// Teleport!
	POS		dst = portal->myDst;

	// We should remain on the same map!
	assert(dst.map() == myMap);

	// Apply the portal's twist.
	dst.myAngle += myAngle;
	dst.myAngle &= 3;

	// Portals have no thickness so we re-apply our original delta!
	// Actually, I changed my mind since then, so it is rather important
	// we actually do dec dx/dy.
	dx -= sdx;
	dy -= sdy;
	rotateToWorld(dx, dy);
	return dst.delta(dx, dy);
    }

    // Find the world delta to apply to our next square.
    dx -= sdx;
    dy -= sdy;
    rotateToWorld(dx, dy);

    // Get our next square...
    POS		dst = *this;

    dst.myX += sdx;
    dst.myY += sdy;

    if (dst.myX < 0 || dst.myX >= dst.room()->width() ||
	dst.myY < 0 || dst.myY >= dst.room()->height())
    {
	// An invalid square!
	dst.myRoomId = -1;
	return dst;
    }

    return dst.delta(dx, dy);
}

void
POS::describeSquare(bool isblind) const
{
    BUF		 buf;
    buf.reference(defn().legend);
    ITEMLIST	 itemlist;
    if (!isblind)
    {
	msg_format("You see %O.", 0, buf);
	
	if (mob())
	    msg_format("You see %O.", 0, mob());

	allItems(itemlist);
	if (itemlist.entries())
	{
	    BUF		msg;
	    msg.strcpy("You see ");
	    for (int i = 0; i < itemlist.entries(); i++)
	    {
		if (i)
		{
		    if (i == itemlist.entries()-1)
			msg.strcat(" and ");
		    else
			msg.strcat(", ");
		}

		msg.strcat(itemlist(i)->getArticleName());
	    }

	    msg.strcat(".");
	    msg_report(msg);
	}
    }
}

///
/// ROOM functions
///

ROOM::ROOM()
{
    myTiles = myFlags = 0;
    myDists = 0;
    myId = -1;
    myColor[0] = myColor[1] = myColor[2] = 64;
    myFragment = 0;
    myType = ROOMTYPE_NONE;

    memset(myJacob, 0, sizeof(int) * 4);
}

ROOM::ROOM(const ROOM &room)
{
    myTiles = myFlags = 0;
    myDists = 0;
    *this = room;
}

void
ROOM::save(ostream &os) const
{
    int32		val;
    int			i;

    val = myId;
    os.write((const char *) &val, sizeof(int32));

    val = myType;
    os.write((const char *) &val, sizeof(int32));

    val = myWidth;
    os.write((const char *) &val, sizeof(int32));
    val = myHeight;
    os.write((const char *) &val, sizeof(int32));

    val = myItems.entries();
    os.write((const char *) &val, sizeof(int32));
    for (i = 0; i < myItems.entries(); i++)
    {
	myItems(i)->save(os);
    }
    val = myMobs.entries();
    os.write((const char *) &val, sizeof(int32));
    for (i = 0; i < myMobs.entries(); i++)
    {
	myMobs(i)->save(os);
    }
    val = myPortals.entries();
    os.write((const char *) &val, sizeof(int32));
    for (i = 0; i < myPortals.entries(); i++)
    {
	myPortals(i).save(os);
    }

    os.write((const char *) myTiles, myWidth * myHeight);
    os.write((const char *) myFlags, myWidth * myHeight);

    os.write((const char *) myColor, 3);
    for (i = 0; i < 4; i++)
    {
	val = myJacob[i];
	os.write((const char *) &val, sizeof(int32));
    }
}

ROOM *
ROOM::load(istream &is)
{
    ROOM		*result;
    int32		val;
    int			i, w, h, n;

    result = new ROOM();

    is.read((char *) &val, sizeof(int32));
    result->myId = val;
    is.read((char *) &val, sizeof(int32));
    result->myType = (ROOMTYPE_NAMES) val;

    is.read((char *) &val, sizeof(int32));
    w = val;
    is.read((char *) &val, sizeof(int32));
    h = val;

    result->resize(w, h);

    is.read((char *) &val, sizeof(int32));
    n = val;
    for (i = 0; i < n; i++)
    {
	result->myItems.append(ITEM::load(is));
    }
    is.read((char *) &val, sizeof(int32));
    n = val;
    for (i = 0; i < n; i++)
    {
	result->myMobs.append(MOB::load(is));
    }
    is.read((char *) &val, sizeof(int32));
    n = val;
    for (i = 0; i < n; i++)
    {
	result->myPortals.append(PORTAL::load(is));
    }

    is.read( (char *) result->myTiles, w * h);
    is.read( (char *) result->myFlags, w * h);

    is.read( (char *) result->myColor, 3);

    for (i = 0; i < 4; i++)
    {
	is.read((char *) &val, sizeof(int32));
	result->myJacob[i] = val;
    }

    return result;
}

ROOM &
ROOM::operator=(const ROOM &room)
{
    if (this == &room)
	return *this;

    deleteContents();

    myFragment = room.myFragment;

    resize(room.myWidth, room.myHeight);

    memcpy(myTiles, room.myTiles, sizeof(u8) * myWidth * myHeight);
    memcpy(myFlags, room.myFlags, sizeof(u8) * myWidth * myHeight);
    memcpy(myDists, room.myDists, DISTMAP_CACHE * sizeof(int) * myWidth * myHeight);
    memcpy(myColor, room.myColor, sizeof(u8) * 3);
    memcpy(myJacob, room.myJacob, sizeof(int) * 2 * 2);

    int		i;
    for (i = 0; i < room.myMobs.entries(); i++)
    {
	myMobs.append(room.myMobs(i)->copy());
    }
    for (i = 0; i < room.myItems.entries(); i++)
    {
	myItems.append(room.myItems(i)->copy());
    }
    myPortals = room.myPortals;
    myId = room.myId;
    myType = room.myType;

    // This usually requires some one to explicity call setMap()
    myMap = 0;

    return *this;
}


ROOM::~ROOM()
{
    deleteContents();
}

void
ROOM::deleteContents()
{
    int		i;

    // We must do a backwards loop as invoking delete on a MOB
    // will then invoke removeMob() on ourselves.
    for (i = myMobs.entries(); i --> 0; )
    {
	myMobs(i)->clearAllPos();
	delete myMobs(i);
    }
    myMobs.clear();
    // Must do a backwards loop as delete item will invoke removeItem
    for (i = myItems.entries(); i --> 0; )
    {
	myItems(i)->clearAllPos();
	delete myItems(i);
    }
    myItems.clear();

    myPortals.clear();

    delete [] myTiles;
    delete [] myFlags;
    myTiles = myFlags = 0;

    delete [] myDists;
    myDists = 0;
}

void
ROOM::deleteAllItems()
{
    // Must do a backwards loop as delete item will invoke removeItem
    for (int i = myItems.entries(); i --> 0; )
    {
	myItems(i)->clearAllPos();
	delete myItems(i);
    }
    myItems.clear();
}

TILE_NAMES
ROOM::getTile(int x, int y) const
{
    if (x < 0 || x >= width() || y < 0 || y >= height())
	return TILE_INVALID;

    return (TILE_NAMES) myTiles[x + y * width()];
}

void
ROOM::setTile(int x, int y, TILE_NAMES tile)
{
    myTiles[x + y * width()] = tile;
}

MAPFLAG_NAMES
ROOM::getFlag(int x, int y) const
{
    return (MAPFLAG_NAMES) myFlags[x + y * width()];
}

void
ROOM::setFlag(int x, int y, MAPFLAG_NAMES flag, bool state)
{
    if (state)
    {
	myFlags[x + y*width()] |= flag;
    }
    else
    {
	myFlags[x + y*width()] &= ~flag;
    }
}

void
ROOM::setAllFlags(MAPFLAG_NAMES flag, bool state)
{
    int		x, y;

    for (y = 0; y < height(); y++)
	for (x = 0; x < width(); x++)
	    setFlag(x, y, flag, state);
}

int
ROOM::getDistance(int mapnum, int x, int y) const
{
    return myDists[x + y * width() + mapnum * width() * height()];
}

void
ROOM::setDistance(int mapnum, int x, int y, int dist)
{
    myDists[x + y * width() + mapnum * width() * height()] = dist;
}

void
ROOM::clearDistMap(int mapnum)
{
    memset(&myDists[mapnum * width() * height()], 0xff, sizeof(int) * width() * height());
}

void
ROOM::setMap(MAP *map)
{
    int			i;

    myMap = map;

    for (i = 0; i < myMobs.entries(); i++)
    {
	myMobs(i)->setMap(map);
	map->myMobCount[myMobs(i)->getDefinition()]++;
    }
    for (i = 0; i < myItems.entries(); i++)
    {
	myItems(i)->setMap(map);
    }
    for (i = 0; i < myPortals.entries(); i++)
    {
	PORTAL	p = myPortals(i);
	p.mySrc.setMap(map);
	p.myDst.setMap(map);
	myPortals.set(i, p);
    }
}

void
ROOM::resize(int w, int h)
{
    delete [] myTiles;
    delete [] myFlags;
    delete [] myDists;

    myWidth = w;
    myHeight = h;

    myTiles = new u8[myWidth * myHeight];
    memset(myTiles, 0, myWidth * myHeight);
    myFlags = new u8[myWidth * myHeight];
    memset(myFlags, 0, myWidth * myHeight);
    myDists = new int[myWidth * myHeight * DISTMAP_CACHE];
}

POS
ROOM::buildPos(int x, int y) const
{
    POS		p;

    p.myMap = myMap;
    p.myRoomId = myId;
    p.myX = x;
    p.myY = y;
    p.myAngle = 0;

    return p;
}

POS
ROOM::getRandomPos(POS p, int *n) const
{
    POS		np;
    int		x, y;
    int		ln = 1;

    if (!n)
	n = &ln;

    np.myMap = myMap;
    np.myRoomId = myId;
    np.myAngle = rand_choice(4);

    for (y = 0; y < height(); y++)
    {
	for (x = 0; x < width(); x++)
	{
	    np.myX = x; np.myY = y;

	    if (np.isPassable() && !np.mob())
	    {
		if (!rand_choice(*n))
		{
		    p = np;
		}
		// I hate ++ on * :>
		*n += 1;
	    }
	}
    }

    return p;
}

POS
ROOM::getRandomTile(TILE_NAMES tile, POS p, int *n) const
{
    POS		np;
    int		x, y;
    int		ln = 1;

    if (!n)
	n = &ln;

    np.myMap = myMap;
    np.myRoomId = myId;
    np.myAngle = rand_choice(4);

    for (y = 0; y < height(); y++)
    {
	for (x = 0; x < width(); x++)
	{
	    np.myX = x; np.myY = y;

	    if (np.tile() == tile)
	    {
		if (!rand_choice(*n))
		{
		    p = np;
		}
		// I hate ++ on * :>
		*n += 1;
	    }
	}
    }

    return p;
}

void
ROOM::findAllTiles(TILE_NAMES tile, POSLIST &list) const
{
    POS		np;
    int		x, y;

    np.myMap = myMap;
    np.myRoomId = myId;
    np.myAngle = rand_choice(4);

    for (y = 0; y < height(); y++)
    {
	for (x = 0; x < width(); x++)
	{
	    np.myX = x; np.myY = y;

	    if (np.tile() == tile)
	    {
		list.append(np);
	    }
	}
    }
}

MOB *
ROOM::getMob(int x, int y) const
{
    int			i;

    for (i = 0; i < myMobs.entries(); i++)
    {
	if (myMobs(i)->pos().myX == x && myMobs(i)->pos().myY == y)
	{
	    // Ignore swallowed mobs or we'd attack ourselves.
	    if (myMobs(i)->isSwallowed())
		continue;
	    return myMobs(i);
	}
    }

    return 0;
}

void
ROOM::getAllMobs(int x, int y, MOBLIST &list) const
{
    int			i;

    for (i = 0; i < myMobs.entries(); i++)
    {
	if (myMobs(i)->pos().myX == x && myMobs(i)->pos().myY == y)
	{
	    list.append(myMobs(i));
	}
    }
}

int
ROOM::getConnectedRooms(ROOMLIST &list) const
{
    for (int i = 0; i < myPortals.entries(); i++)
    {
	ROOM	*dst = myPortals(i).myDst.room();

	if (list.find(dst) < 0)
	{
	    list.append(dst);
	}
    }

    return list.entries();
}

void
ROOM::addMob(int x, int y, MOB *mob)
{
    if (myMap)
	myMap->myMobCount[mob->getDefinition()]++;
    myMobs.append(mob);
}
    
void
ROOM::removeMob(MOB *mob)
{
    if (myMap)
	myMap->myMobCount[mob->getDefinition()]--;
    myMobs.removePtr(mob);
}
    

ITEM *
ROOM::getItem(int x, int y) const
{
    int			i;
    ITEM		*corpse = 0;

    for (i = 0; i < myItems.entries(); i++)
    {
	if (myItems(i)->pos().myX == x && myItems(i)->pos().myY == y)
	{
	    if (myItems(i)->getDefinition() == ITEM_CORPSE)
		corpse = myItems(i);
	    else
		return myItems(i);
	}
    }

    return corpse;
}
    
void
ROOM::getAllItems(int x, int y, ITEMLIST &list) const
{
    int			i;

    for (i = 0; i < myItems.entries(); i++)
    {
	if (myItems(i)->pos().myX == x && myItems(i)->pos().myY == y)
	{
	    list.append(myItems(i));
	}
    }
}
    
void
ROOM::addItem(int x, int y, ITEM *item)
{
    for (int i = 0; i < myItems.entries(); i++)
    {
	if (myItems(i)->pos().myX == x && myItems(i)->pos().myY == y)
	{
	    if (item->canStackWith(myItems(i)))
	    {
		myItems(i)->combineItem(item);
		delete item;
		return;
	    }
	}
    }
    myItems.append(item);
}
    
void
ROOM::removeItem(ITEM *item)
{
    myItems.removePtr(item);
}
    
PORTAL *
ROOM::getPortal(int x, int y) const
{
    int			i;

    // Quick test.
    if (!(getFlag(x, y) & MAPFLAG_PORTAL))
	return 0;

    for (i = 0; i < myPortals.entries(); i++)
    {
	if (myPortals(i).mySrc.myX == x && myPortals(i).mySrc.myY == y)
	{
	    return myPortals.rawptr(i);
	}
    }

    return 0;
}

void
ROOM::removePortal(POS dst)
{
    int			i;

    for (i = 0; i < myPortals.entries(); i++)
    {
	if (myPortals(i).myDst == dst)
	{
	    myPortals(i).mySrc.setFlag(MAPFLAG_PORTAL, false);
	    myPortals(i).myDst.setFlag(MAPFLAG_PORTAL, false);
	    myPortals.removeAt(i);
	    i--;
	}
    }
}

void
ROOM::removeAllForeignPortals()
{
    int			i;

    for (i = 0; i < myPortals.entries(); i++)
    {
	if (myPortals(i).myDst.roomType() != type())
	{
	    myPortals(i).mySrc.setFlag(MAPFLAG_PORTAL, false);
	    myPortals(i).myDst.setFlag(MAPFLAG_PORTAL, false);
	    myPortals.removeAt(i);
	    i--;
	}
    }
}

void
ROOM::removeAllPortalsTo(ROOMTYPE_NAMES roomtype)
{
    int			i;

    for (i = 0; i < myPortals.entries(); i++)
    {
	if (myPortals(i).myDst.roomType() == roomtype)
	{
	    myPortals(i).mySrc.setFlag(MAPFLAG_PORTAL, false);
	    myPortals(i).myDst.setFlag(MAPFLAG_PORTAL, false);
	    myPortals.removeAt(i);
	    i--;
	}
    }
}

void
ROOM::removeAllPortals()
{
    myPortals.clear();
}

POS
ROOM::findProtoPortal(int dir)
{
    int		dx, dy, x, y, nfound;
    POS	result;

    rand_getdirection(dir, dx, dy);

    nfound = 0;

    for (y = 0; y < height(); y++)
    {
	for (x = 0; x < width(); x++)
	{
	    if (getTile(x, y) == TILE_PROTOPORTAL ||
	        getTile(x, y) == TILE_MOUNTAINPROTOPORTAL)
	    {
		if (getTile(x+dx, y+dy) == TILE_INVALID &&
		    ( glb_tiledefs[getTile(x-dx, y-dy)].validportalfloor )
		     )
		{
		    nfound++;
		    if (!rand_choice(nfound))
		    {
			result = buildPos(x, y);
		    }
		}
	    }
	}
    }

    if (nfound)
    {
    }
    else
    {
	{
	    cerr << "Failed to locate on map " << "Damn so that is what my fragment was used for!!!" << endl;
	    if (myFragment)
	    {
		cerr << myFragment->myFName.buffer() << endl;
		cerr << "Direction was " << dir << " type is " << glb_roomtypedefs[myType].prefix << endl;
	    }
	}
    }
    return result;
}

POS
ROOM::findUserProtoPortal(int dir)
{
    int		dx, dy, x, y, nfound;
    POS	result;

    rand_getdirection(dir, dx, dy);

    nfound = 0;

    for (y = 0; y < height(); y++)
    {
	for (x = 0; x < width(); x++)
	{
	    if (getTile(x, y) == TILE_USERPROTOPORTAL)
	    {
		if (getTile(x+dx, y+dy) == TILE_INVALID &&
		    ( getTile(x-dx, y-dy) == TILE_FLOOR ||
		      getTile(x-dx, y-dy) == TILE_GRASS ||
		      getTile(x-dx, y-dy) == TILE_FOREST
		     ))
		{
		    nfound++;
		    if (!rand_choice(nfound))
		    {
			result = buildPos(x, y);
		    }
		}
	    }
	}
    }

    if (nfound)
    {
    }
    else
    {
	{
	    cerr << "Failed to locate on map " << "Damn so that is what my fragment was used for!!!" << endl;
	    if (myFragment)
	    {
		cerr << myFragment->myFName.buffer() << endl;
		cerr << "Direction was " << dir << " type is " << glb_roomtypedefs[myType].prefix << endl;
	    }
	}
    }
    return result;
}

POS
ROOM::findCentralPos() const
{
    POS		center;

    center = buildPos(width()/2, height()/2);
    return spiralSearch(center, 0, false);
}

#define TEST_TILE(x, y) \
if ( glb_tiledefs[getTile(x, y)].ispassable && (!avoidmob || !getMob(x, y)) )\
    return buildPos(x, y);

POS
ROOM::spiralSearch(POS start, int startradius, bool avoidmob) const
{
    // Spiral search for passable tile from the center.
    int		x = start.myX, y = start.myY;
    int		ir, r, maxr = MAX(width(), height())+1;
    POS		result;

    for (r = startradius; r < maxr; r++)
    {
	for (ir = -r; ir <= r; ir++)
	{
	    TEST_TILE(x+ir, y+r)
	    TEST_TILE(x+ir, y-r)
	    TEST_TILE(x+r, y+ir)
	    TEST_TILE(x-r, y+ir)
	}
    }

    assert(!"No valid squares in a room.");
    return result;
}

ATLAS_NAMES
ROOM::getAtlas() const 
{ 
    return myMap ? myMap->getAtlas() : ATLAS_NONE; 
}

bool
ROOM::buildPortal(POS a, int dira, POS b, int dirb, bool settiles)
{
    PORTAL		 patob, pbtoa;

    if (!a.valid() || !b.valid())
	return false;

    patob.mySrc = a.delta4Direction(dira);
    patob.myDst = b;

    pbtoa.mySrc = b.delta4Direction(dirb);
    pbtoa.myDst = a;

    if (settiles)
    {
	patob.mySrc.setTile(TILE_SOLIDWALL);
	pbtoa.mySrc.setTile(TILE_SOLIDWALL);
    }

    patob.mySrc.myAngle = 0;
    pbtoa.mySrc.myAngle = 0;

    if (settiles)
    {
	patob.myDst.setTile((TILE_NAMES) glb_roomtypedefs[patob.myDst.roomType()].portalfloor);
	pbtoa.myDst.setTile((TILE_NAMES) glb_roomtypedefs[pbtoa.myDst.roomType()].portalfloor);
    }
    patob.myDst.setFlag(MAPFLAG_PORTAL, true);
    patob.mySrc.setFlag(MAPFLAG_PORTAL, true);
    pbtoa.myDst.setFlag(MAPFLAG_PORTAL, true);
    pbtoa.mySrc.setFlag(MAPFLAG_PORTAL, true);

    patob.myDst.myAngle = (dira - dirb + 2) & 3;
    pbtoa.myDst.myAngle = (dirb - dira + 2) & 3;

    a.room()->myPortals.append(patob);
    b.room()->myPortals.append(pbtoa);

    return true;
}



bool
ROOM::link(ROOM *a, int dira, ROOM *b, int dirb)
{
    POS		aseed, bseed;
    POS		an, bn;

    aseed = a->findProtoPortal(dira);
    bseed = b->findProtoPortal(dirb);

    if (!aseed.valid() || !bseed.valid())
	return false;

    // This code is to ensure we do as thick a match
    // as possible.
#if 1
    // Grow to the top.
    an = aseed;
    while (an.tile() == TILE_PROTOPORTAL)
    {
	aseed = an;
	an = an.delta4Direction(dira+1);
    }
    bn = bseed;
    while (bn.tile() == TILE_PROTOPORTAL)
    {
	bseed = bn;
	bn = bn.delta4Direction(dirb-1);
    }

#if 1
    int asize = 0, bsize = 0;

    an = aseed;
    while (an.tile() == TILE_PROTOPORTAL)
    {
	asize++;
	an = an.delta4Direction(dira-1);
    }
    bn = bseed;
    while (bn.tile() == TILE_PROTOPORTAL)
    {
	bsize++;
	bn = bn.delta4Direction(dirb+1);
    }

    if (asize > bsize)
    {
	int		off = rand_choice(asize - bsize);

	while (off)
	{
	    off--;
	    aseed = aseed.delta4Direction(dira-1);
	    assert(aseed.tile() == TILE_PROTOPORTAL);
	}
    }
    else if (bsize > asize)
    {
	int		off = rand_choice(bsize - asize);

	while (off)
	{
	    off--;
	    bseed = bseed.delta4Direction(dirb+1);
	    assert(bseed.tile() == TILE_PROTOPORTAL);
	}
    }
#endif
#endif

    ROOM::buildPortal(aseed, dira, bseed, dirb);

    // Grow our portal in both directions as far as it will go.
    an = aseed.delta4Direction(dira+1);
    bn = bseed.delta4Direction(dirb-1);
    while (an.tile() == TILE_PROTOPORTAL && bn.tile() == TILE_PROTOPORTAL)
    {
	ROOM::buildPortal(an, dira, bn, dirb);
	an = an.delta4Direction(dira+1);
	bn = bn.delta4Direction(dirb-1);
    }

    an = aseed.delta4Direction(dira-1);
    bn = bseed.delta4Direction(dirb+1);
    while (an.tile() == TILE_PROTOPORTAL && bn.tile() == TILE_PROTOPORTAL)
    {
	ROOM::buildPortal(an, dira, bn, dirb);
	an = an.delta4Direction(dira-1);
	bn = bn.delta4Direction(dirb+1);
    }

    return true;
}

///
/// FRAGMENT definition and functions
///


FRAGMENT::FRAGMENT(const char *fname)
{
    ifstream		is(fname);
    char		line[500];
    PTRLIST<char *>	l;
    int			i, j;

    myFName.strcpy(fname);

    while (is.getline(line, 500))
    {
	text_striplf(line);

	// Ignore blank lines...
	if (line[0])
	    l.append(strdup(line));
    }

    if (l.entries() < 1)
    {
	cerr << "Empty map fragment " << fname << endl;
	exit(-1);
    }

    // Find smallest non-whitespace square.
    int			minx, maxx, miny, maxy;

    for (miny = 0; miny < l.entries(); miny++)
    {
	if (text_hasnonws(l(miny)))
	    break;
    }

    for (maxy = l.entries(); maxy --> 0; )
    {
	if (text_hasnonws(l(maxy)))
	    break;
    }

    if (miny > maxy)
    {
	cerr << "Blank map fragment " << fname << endl;
	exit(-1);
    }

    minx = strlen(l(miny));
    maxx = 0;
    for (i = miny; i <= maxy; i++)
    {
	minx = MIN(minx, text_firstnonws(l(i)));
	maxx = MAX(maxx, text_lastnonws(l(i)));
    }

    // Joys of rectangles with inconsistent exclusivity!
    // pad everything by 1.
    myW = maxx - minx + 1 + 2;
    myH = maxy - miny + 1 + 2;

    myTiles = new u8 [myW * myH];

    memset(myTiles, ' ', myW * myH);

    for (i = 1; i < myH-1; i++)
    {
	if (strlen(l(i+miny-1)) < myW + minx - 2)
	{
	    cerr << "Short line in fragment " << fname << endl;
	    exit(-1);
	}
	for (j = 1; j < myW-1; j++)
	{
	    myTiles[i * myW + j] = l(i+miny-1)[j+minx-1];
	}
    }

    for (i = 0; i < l.entries(); i++)
	free(l(i));
}

FRAGMENT::~FRAGMENT()
{
    delete [] myTiles;
}

void
FRAGMENT::swizzlePos(int &x, int &y, int orient) const
{
    if (orient & ORIENT_FLIPX)
    {
	x = myW - x - 1;
    }

    if (orient & ORIENT_FLIPY)
    {
	y = myH - y - 1;
    }

    if (orient & ORIENT_FLOP)
    {
	int	t = y;
	y = x;
	x = t;
    }
}

ROOM *
FRAGMENT::buildRoom(MAP *map, ROOMTYPE_NAMES type) const
{
    ROOM	*room;
    int		 x, y, rx, ry;
    MOB		*mob;
    ITEM	*item;
    TILE_NAMES	 tile;
    MAPFLAG_NAMES flag;
    int		 depth = 1;
    int		 orient = 0;

    if (glb_roomtypedefs[type].randomorient)
	orient = rand_choice(7);

    room = new ROOM();
    room->myFragment = this;

    if (orient & ORIENT_FLOP)
	room->resize(myH, myW);
    else
	room->resize(myW, myH);
    room->setMap(map);
    room->myType = type;

    room->myId = map->myRooms.entries();
    map->myRooms.append(room);

    int		ngold = 0;
    int		nitem = 0;
    int		nmob = 0;
    
    for (y = 0; y < myH; y++)
    {
	for (x = 0; x < myW; x++)
	{
	    rx = x;
	    ry = y;
	    swizzlePos(rx, ry, orient);

	    tile = TILE_INVALID;
	    flag = MAPFLAG_NONE;
	    switch (myTiles[x + y * myW])
	    {
		case '1':
		    mob = MOB::create(MOB_KING);
		    mob->move(room->buildPos(rx, ry));
		    tile = TILE_THRONE;
		    break;
		case '2':
		    mob = MOB::create(MOB_ADVISOR_PEACE);
		    mob->move(room->buildPos(rx, ry));
		    tile = TILE_FLOOR;
		    break;
		case '3':
		    mob = MOB::create(MOB_ADVISOR_WAR);
		    mob->move(room->buildPos(rx, ry));
		    tile = TILE_FLOOR;
		    break;
		case '8':
		    mob = MOB::create(MOB_GUARD);
		    mob->move(room->buildPos(rx, ry));
		    tile = TILE_FLOOR;
		    break;
		case '9':
		    mob = MOB::create(MOB_SERVANT);
		    mob->move(room->buildPos(rx, ry));
		    tile = TILE_FLOOR;
		    break;
		case 'A':		// any monster
		    if (nmob < glb_roomtypedefs[type].nmob)
		    {
			mob = MOB::createNPC(room->getAtlas());
			if (mob)
			    mob->move(room->buildPos(rx, ry));
			nmob ++;
		    }
		    tile = TILE_FLOOR;
		    break;

		case 'B':		// boss monster
		    mob = MOB::create((MOB_NAMES)glb_roomtypedefs[type].boss);
		    if (mob)
		    {
			mob->move(room->buildPos(rx, ry));
			item = ITEM::create((ITEM_NAMES) glb_questdefs[glbCurrentQuest].questitem);
			if (item)
			    mob->addItem(item);
		    }

		    tile = TILE_FLOOR;
		    break;

		case '(':
		    item = ITEM::createRandom(room->getAtlas());
		    if (item)
			item->move(room->buildPos(rx, ry));
		    nitem++;
		    tile = TILE_FLOOR;
		    break;

		case 'S':
		    tile = TILE_SECRETDOOR;
		    break;

		case '%':
		{
		    ITEM_NAMES		list[5] =
		    {
			ITEM_SAUSAGES,
			ITEM_BREAD,
			ITEM_MUSHROOMS,
			ITEM_APPLE,
			ITEM_PICKLE
		    };
		    item = ITEM::create(list[rand_choice(5)]);
		    item->move(room->buildPos(rx, ry));
		    tile = TILE_FLOOR;
		    break;
		}

		case 'Q':
		    tile = TILE_MEDITATIONSPOT;
		    break;

		case '$':
		    item = ITEM::create(ITEM_GOLD);
		    // average 13, median 15!
		    item->setStackCount(rand_roll(15, 1));
		    item->move(room->buildPos(rx, ry));
		    ngold++;
		    tile = TILE_FLOOR;
		    break;

		case ';':
		    if (type == ROOMTYPE_THRONE)
			tile = TILE_LAVENDERCARPET;
		    else if (type == ROOMTYPE_GOBLIN_FINISH)
			tile = TILE_GREENCARPET;
		    else
			tile = TILE_PATH;
		    break;

		case '.':
		    tile = TILE_FLOOR;
		    break;

		case 'x':
		    tile = TILE_DOOR;
		    break;

		case 'K':
		    tile = TILE_STATUE;
		    break;

		case '_':
		    tile = TILE_ALTAR;
		    break;

		case '-':
		    tile = TILE_DESERT;
		    break;

		case '#':
		    if (type == ROOMTYPE_STRATEGY)
			tile = TILE_CITY_RED;
		    else
			tile = TILE_WALL;
		    break;

		case '&':
		    if (type == ROOMTYPE_CRYPT_START)
			tile = TILE_BAREFOREST;
		    else
			tile = TILE_FOREST;
		    break;

		case '*':
		    if (type == ROOMTYPE_STRATEGY)
			tile = TILE_CITY_VIOLET;
		    else
			tile = TILE_FIRE;
		    break;

		case 'o':
		    tile = TILE_WINDOW;
		    break;

		case 'M':
		    tile = TILE_MAPTABLE;
		    break;

		case 'O':
		    tile = TILE_TABLE;
		    break;

		case 'h':
		    tile = TILE_CHAIR;
		    break;

		case ']':
		    tile = TILE_CHAIR;
		    break;

		case '+':
		    tile = TILE_PROTOPORTAL;
		    break;

		case '<':
		    tile = TILE_UPSTAIRS;
		    break;

		case '>':
		    tile = TILE_DOWNSTAIRS;
		    break;

		case '!':
		    tile = TILE_PORTCULLIS;
		    break;
		case ' ':
		    tile = TILE_INVALID;
		    break;
		case '"':
		    tile = TILE_FIELD;
		    break;
		case ',':
		    tile = TILE_GRASS;
		    break;
		case '~':
		    tile = TILE_WATER;
		    break;
		case '^':
		    tile = TILE_MOUNTAIN;
		    break;
		case 'P':
		    tile = TILE_USERPROTOPORTAL;
		    break;
		case 'V':
		    tile = TILE_ICEMOUNTAIN;
		    break;
		case 'W':
		    tile = TILE_THRONE;
		    break;
		case '=':
		    if (type == ROOMTYPE_STRATEGY)
			tile = TILE_BRIDGE;
		    else
			tile = TILE_BED;
		    break;
		default:
		    fprintf(stderr, "Unknown map char %c\r\n",
			    myTiles[x + y * myW]);
	    }

	    // While I like the traps, they need to be rare enough
	    // not to force people to search all the time.  But then
	    // they just become surprise insta-deaths?
	    // So, compromise: traps do very little damage so act
	    // primarily as flavour.  Which in traditional roguelike
	    // fashion is always excused.
	    if (tile == TILE_FLOOR && 
		!rand_choice(500) && type != ROOMTYPE_THRONE)
	    {
		flag = MAPFLAG_TRAP;
	    }
	    room->setTile(rx, ry, tile);
	    room->setFlag(rx, ry, flag, true);
	}
    }

    POS	pos;

    while (ngold < glb_roomtypedefs[type].ngold)
    {
	pos = room->getRandomTile(TILE_FLOOR, POS(), 0);
	if (pos.valid())
	{
	    item = ITEM::create(ITEM_GOLD);
	    item->setStackCount(rand_roll(15, 1));
	    item->move(pos);
	}
	ngold++;
    }
    while (nitem < glb_roomtypedefs[type].nitem)
    {
	pos = room->getRandomTile(TILE_FLOOR, POS(), 0);
	if (pos.valid())
	{
	    item = ITEM::createRandom(room->getAtlas());
	    if (item)
		item->move(pos);
	}
	nitem++;
    }
    while (nmob < glb_roomtypedefs[type].nmob)
    {
	pos = room->getRandomTile(TILE_FLOOR, POS(), 0);
	if (!pos.valid())
	    pos = room->getRandomTile(TILE_GRASS, POS(), 0);
	if (!pos.valid())
	    pos = room->getRandomTile(TILE_FOREST, POS(), 0);
	if (pos.valid())
	{
	    mob = MOB::createNPC(room->getAtlas());

	    if (mob)
		mob->move(pos);
	}
	nmob ++;
    }

    room->setColor(glb_roomtypedefs[type].wall_r,
		   glb_roomtypedefs[type].wall_g,
		   glb_roomtypedefs[type].wall_b);

    return room;
}



static ATOMIC_INT32 glbMapId;

///
/// MAP functions
///
MAP::MAP(ATLAS_NAMES atlas, MOB *avatar, DISPLAY *display)
{
    myRefCnt.set(0);

    myAtlas = atlas;

    myUniqueId = glbMapId.add(1);
    myFOVCache = 0;
    myAllowDelays = false;

    MOB_NAMES mob;
    FOREACH_MOB(mob)
    {
	myMobCount[mob] = 0;
    }

    FRAGMENT		*frag;

    int			 i;
    ROOMTYPE_NAMES	 roomtype;

    // Build the entrance.

    roomtype = (ROOMTYPE_NAMES) glb_atlasdefs[atlas].start_roomtype;

    frag = theFragRooms[roomtype](rand_choice(theFragRooms[roomtype].entries()));
    frag->buildRoom(this, roomtype);

    // Now build the mid rooms.
    roomtype = (ROOMTYPE_NAMES) glb_atlasdefs[atlas].mid_roomtype;
    if (roomtype != ROOMTYPE_NONE)
    {
	ROOM		*exitroom;
	int		 exitdir;

	buildSquare(roomtype, myRooms(0), 2,
				    exitroom, exitdir,
				    4, 4);
				    //1, 1);

	// And build the final room...
	roomtype = (ROOMTYPE_NAMES) glb_atlasdefs[atlas].end_roomtype;
	buildSquare(roomtype, exitroom, exitdir,
				    exitroom, exitdir,
				    1, 1);

    }

    if (avatar)
    {
	myAvatar = avatar;
	POS		goal;

	goal = myRooms(0)->getRandomTile(TILE_UPSTAIRS, POS(), 0);
	if (!goal.valid())
	    goal = myRooms(0)->getRandomPos(POS(), 0);
	else
	{
	    // Find non-mob spot at least one step from the ladder
	    // We want to start on the ladder so belay that!
	    goal = myRooms(0)->spiralSearch(goal, 0, true);
	}
	// Keep us oriented consistently.
	//goal = goal.rotate(rand_choice(4));
	goal.setAngle(0);
	avatar->move(goal);

	// Ensure our avatar is legal and flagged.
	avatar->updateEquippedItems();
    }

    myDisplay = display;

    for (i = 0; i < DISTMAP_CACHE; i++)
    {
	myDistMapWeight[i] = 0.0;
    }

    FOREACH_ROOMTYPE(roomtype)
    {
	glbRoomDiscovered[roomtype] = false;
	glbRoomLooted[roomtype] = false;
    }

    SPELL_NAMES		spell;
    FOREACH_SPELL(spell)
    {
	glbManaSpent[spell] = 0;
    }
}

MAP::MAP(const MAP &map)
{
    myRefCnt.set(0);
    myFOVCache = 0;
    myAllowDelays = false;
    *this = map;
}

MAP &
MAP::operator=(const MAP &map)
{
    int		i;

    if (this == &map)
	return *this;

    deleteContents();

    MOB_NAMES mob;
    FOREACH_MOB(mob)
    {
	myMobCount[mob] = 0;
    }

    for (i = 0; i < map.myRooms.entries(); i++)
    {
	myRooms.append(new ROOM(*map.myRooms(i)));
	myRooms(i)->setMap(this);
    }

    for (i = 0; i < 2; i++)
    {
	myUserPortal[i] = map.myUserPortal[i];
	myUserPortal[i].setMap(this);
	myUserPortalDir[i] = map.myUserPortalDir[i];
    }

    for (i = 0; i < DISTMAP_CACHE; i++)
    {
	myDistMapCache[i] = map.myDistMapCache[i];
	myDistMapCache[i].setMap(this);
	// This is our decay coefficient so stuff eventually will
	// flush out regardless of how popular they were.
	myDistMapWeight[i] = map.myDistMapWeight[i] * 0.75;
    }

    myAvatar = findAvatar();
    
    myUniqueId = map.getId();

    myAtlas = map.getAtlas();

    myDisplay = map.myDisplay;

    // We need to build our own FOV cache to ensure poses refer
    // to us.
    myFOVCache = 0;

    return *this;
}

void
MAP::deleteContents()
{
    int		i;

    for (i = 0; i < myRooms.entries(); i++)
    {
	delete myRooms(i);
    }
    myRooms.clear();

    delete myFOVCache;
    myFOVCache = 0;
}

MAP::~MAP()
{
    deleteContents();
}

void
MAP::incRef()
{
    myRefCnt.add(1);
}

void
MAP::decRef()
{
    int		nval;
    nval = myRefCnt.add(-1);

    if (nval <= 0)
	delete this;
}

void
MAP::buildSquare(ROOMTYPE_NAMES roomtype,
	    ROOM *entrance, int enterdir,
	    ROOM *&exit, int &exitdir,
	    int w, int h)
{
    ROOM		**rooms;

    rooms = new ROOM *[w * h];

    for (int y = 0; y < h; y++)
	for (int x = 0; x < w; x++)
	{
	    FRAGMENT		*frag;

	    frag = theFragRooms[roomtype](rand_choice(theFragRooms[roomtype].entries()));
	    rooms[x + y * w] = frag->buildRoom(this, roomtype);
	}

    // Vertical links
    for (int y = 0; y < h-1; y++)
	for (int x = 0; x < w; x++)
	{
	    ROOM::link( rooms[x + y * w], 2,
			rooms[x + (y+1)*w], 0 );
	}

    // Horizontal links
    for (int y = 0; y < h; y++)
	for (int x = 0; x < w-1; x++)
	{
	    ROOM::link( rooms[x + y * w], 1,
			rooms[x+1 + y * w], 3 );
	}

    // Add mandatory items.
    for (const char *itemid = glb_roomtypedefs[roomtype].mandatoryitem;
	 *itemid;
	 itemid++)
    {
	ITEM_NAMES		itemname = (ITEM_NAMES) *itemid;

	if (itemname != ITEM_NONE)
	{
	    POS		 pos;
	    ROOM	*room = rooms[rand_choice(w*h)];

	    pos = room->getRandomTile(TILE_FLOOR, POS(), 0);
	    if (pos.valid())
	    {
		ITEM	*item = ITEM::create(itemname);
		if (item)
		    item->move(pos);
	    }
	}
    }

    // Link in the entrance.
    // Since we are all orientation invariant, we can always link into
    // our LHS.
    int x, y;
    x = 0;
    y = rand_choice(h);
    ROOM::link( entrance, enterdir,
		rooms[x + y * w], 3 );
    // Now choose an exit...
    int		v = rand_choice( (w-1) * 2 + h );
    if (v < w-1)
    {
	exit = rooms[v + 1 + 0 * w];
	exitdir = 0;
    }
    else
    {
	v -= w-1;
	if (v < w -1)
	{
	    exit = rooms[v + 1 + (h-1) * w];
	    exitdir = 2;
	}
	else
	{
	    v -= w - 1;
	    assert(v < h);
	    exit = rooms[(w-1) + v * w];
	    exitdir = 1;
	}
    }

    delete [] rooms;
}

int
MAP::getNumMobs() const
{
    int		i, total = 0;

    for (i = 0; i < myRooms.entries(); i++)
    {
	total += myRooms(i)->myMobs.entries();
    }

    return total;
}


int
MAP::getAllItems(ITEMLIST &list) const
{
    for (int i = 0; i < myRooms.entries(); i++)
    {
	list.append(myRooms(i)->myItems);
    }

    return list.entries();
}

int
MAP::getAllMobs(MOBLIST &list) const
{
    for (int i = 0; i < myRooms.entries(); i++)
    {
	list.append(myRooms(i)->myMobs);
    }

    return list.entries();
}

int
MAP::getAllMobsOfType(MOBLIST &list, MOB_NAMES type) const
{
    for (int i = 0; i < myRooms.entries(); i++)
    {
	for (int j = 0; j < myRooms(i)->myMobs.entries(); j++)
	{
	    if (myRooms(i)->myMobs(j)->getDefinition() == type)
		list.append(myRooms(i)->myMobs(j));
	}
    }

    return list.entries();
}

void
MAP::killAllButAvatar() 
{
    for (int i = 0; i < myRooms.entries(); i++)
    {
	for (int j = myRooms(i)->myMobs.entries(); j --> 0; )
	{
	    if (!myRooms(i)->myMobs(j)->isAvatar())
		myRooms(i)->myMobs(j)->kill();
	}
    }
}

void
MAP::lootAllValuable() 
{
    for (int i = 0; i < myRooms.entries(); i++)
    {
	for (int j = myRooms(i)->myItems.entries(); j --> 0; )
	{
	    ITEM		*item = myRooms(i)->myItems(j);

	    if (item->getDefinition() != ITEM_CORPSE)
	    {
		myRooms(i)->removeItem(item);
		delete item;
	    }
	}
    }
}

POS
MAP::getRandomTile(TILE_NAMES tile) const
{
    int			n = 1;
    POS			p;

    for (int i = 0; i < myRooms.entries(); i++)
    {
	p = myRooms(i)->getRandomTile(tile, p, &n);
    }
    return p;
}

void
MAP::findAllTiles(TILE_NAMES tile, POSLIST &poslist) const
{
    for (int i = 0; i < myRooms.entries(); i++)
    {
	myRooms(i)->findAllTiles(tile, poslist);
    }
}

void
MAP::setAllFlags(MAPFLAG_NAMES flag, bool state)
{
    int		i;

    for (i = 0; i < myRooms.entries(); i++)
    {
	myRooms(i)->setAllFlags(flag, state);
    }
}

int
MAP::lookupDistMap(POS goal) const
{
    int		i;

    for (i = 0; i < DISTMAP_CACHE; i++)
    {
	if (goal == myDistMapCache[i])
	{
	    // Successful find!
	    myDistMapWeight[i] += 1.0;
	    return i;
	}
    }
    return -1;
}

int
MAP::allocDistMap(POS goal)
{
    int			i;
    double		bestweight = 1e32;
    int			bestindex = 0;

    for (i = 0; i < DISTMAP_CACHE; i++)
    {
	// Trivial success!
	if (!myDistMapCache[i].valid())
	{
	    bestindex = i;
	    break;
	}

	if (myDistMapWeight[i] < bestweight)
	{
	    bestindex = i;
	    bestweight = myDistMapWeight[i];
	}
    }

    myDistMapWeight[bestindex] = 1.0;
    myDistMapCache[bestindex] = goal;

    clearDistMap(bestindex);

    return bestindex;
}

void
MAP::clearDistMap(int distmap)
{
    int			i;

    for (i = 0; i < myRooms.entries(); i++)
    {
	myRooms(i)->clearDistMap(distmap);
    }
}

int
MAP::buildDistMap(POS goal)
{
    POSLIST		stack;
    POS			p, np;
    int			dist, dx, dy;
    int			distmap;

    assert(goal.valid());

    stack.setCapacity(1000);

    distmap = lookupDistMap(goal);
    if (distmap >= 0)
	return distmap;

#ifdef DO_TIMING
    cerr << "Build distance map" << endl;
#endif

    distmap = allocDistMap(goal);

    goal.setDistance(distmap, 0);

    stack.append(goal);
    while (stack.entries())
    {
	p = stack.removeFirst();

	dist = p.getDistance(distmap);
	dist++;

	FORALL_8DIR(dx, dy)
	{
	    np = p.delta(dx, dy);

	    if (!np.valid())
		continue;

	    if (!np.isPassable())
		continue;

	    if (np.getDistance(distmap) >= 0)
		continue;

	    np.setDistance(distmap, dist);

	    stack.append(np);
	}
    }
    return distmap;
}

void
MAP::addDeadMob(MOB *mob)
{
    if (mob)
    {
	myDeadMobs.append(mob);
    }
}

void
MAP::reapMobs()
{
    for (int i = 0; i < myDeadMobs.entries(); i++)
	delete myDeadMobs(i);

    myDeadMobs.clear();
}

bool
MAP::requestDelayedMove(MOB *mob, int dx, int dy)
{
    if (!myAllowDelays)
	return false;

    myDelayMobList.append( MOBDELAY(mob, dx, dy) );
    return true;
}

void
mapAddAllToFinal(MOB *mob, MOBDELAYLIST &final, const MOBDELAYLIST &list)
{
    int		idx;

    if (!mob->isDelayMob())
	return;

    idx = mob->delayMobIdx();
    final.append(list(idx));
    mob->setDelayMob(false, -1);

    for (int j = 0; j < mob->collisionSources().entries(); j++)
    {
	MOB	*parent;

	parent = mob->collisionSources()(j);
	parent->setCollisionTarget(0);

	mapAddAllToFinal(parent, final, list);
    }
}

MOB *
mapFindCycle(MOB *mob)
{
    // Yay for wikipedia!
    MOB		*tortoise, *hare;

    tortoise = mob;
    hare = tortoise->collisionTarget();

    while (hare)
    {
	if (tortoise == hare)
	    return tortoise;		// Tortoise always wins :>
	tortoise = tortoise->collisionTarget();
	if (!tortoise)
	    return 0;
	hare = hare->collisionTarget();
	if (!hare)
	    return 0;
	hare = hare->collisionTarget();
	if (!hare)
	    return 0;
    }
    return 0;
}

void
mapBuildContactGraph(const MOBDELAYLIST &list)
{
    int		i;

    // Clear our old tree.
    for (i = 0; i < list.entries(); i++)
    {
	MOB		*mob = list(i).myMob;
	mob->clearCollision();
    }
    // Build our tree.
    for (i = 0; i < list.entries(); i++)
    {
	MOB		*mob = list(i).myMob;
	if (!mob->alive() || !mob->isDelayMob())
	    continue;
	mob->setCollisionTarget(mob->pos().delta( list(i).myDx,
						  list(i).myDy ).mob() );
    }
}

void
mapResolveCycle(MOB *mob)
{
    MOB		*next, *start;
    MOBLIST	cycle;
    POSLIST	goals;

    // Compute the cycle
    start = mob;
    do
    {
	next = mob->collisionTarget();

	// Disconnect anyone pointing to me.
	for (int i = 0; i < mob->collisionSources().entries(); i++)
	{
	    mob->collisionSources()(i)->setCollisionTarget(0);
	}

	cycle.append(mob);
	// We can't use next->pos as next will be null when we complete
	// our loop because the above code will disconnect the back pointer!
	// Thus the goal store *our* position.
	goals.append(mob->pos());
	mob->setDelayMob(false, -1);

	mob = next;
    } while (mob && mob != start);


    // We do this in two steps to avoid any potential issues of
    // the underlying map not liking two mobs on the same square.

    // Yank all the mobs off the map
    for (int i = 0; i < cycle.entries(); i++)
    {
	cycle(i)->setPos(POS());
    }

    // And drop them back on their goal pos.
    for (int i = 0; i < cycle.entries(); i++)
    {
	// The goals are off by one, everyone wants to get to the
	// next goal
	cycle(i)->setPos(goals((i + 1) % cycle.entries()));
    }
}

bool
mapRandomPushMob(MOB *mob, int pdx, int pdy)
{
    POSLIST		pushlist;
    POS			target, p, nt;

    pushlist.append(mob->pos());

    // We demand no pushbacks
    // So we do something like 
    int			vdx[3], vdy[3];

    if (pdx && pdy)
    {
	// Diagonal!
	vdx[0] = pdx;
	vdy[0] = pdy;
	vdx[1] = pdx;
	vdy[1] = 0;
	vdx[2] = 0;
	vdy[2] = pdy;
    }
    else
    {
	// Horizontal!
	// Too clever wall sliding code
	int			sdx = !pdx;
	int			sdy = !pdy;

	vdx[0] = pdx;
	vdy[0] = pdy;
	vdx[1] = pdx-sdx;
	vdy[1] = pdy-sdy;
	vdx[2] = pdx+sdx;
	vdy[2] = pdy+sdy;
    }

    target = mob->pos().delta(pdx, pdy);
    while (target.mob())
    {
	bool		found = false;
	int		nfound;

	if (pushlist.find(target) >= 0)
	{
	    // We hit an infinite loop, no doubt due to a looped room.
	    return false;
	}

	pushlist.append(target);

	// Find a place to push target to.
	// We only ever move in vdx possible directions.  We also
	// verify we don't end up beside the mob.  Thanks to looping rooms
	// this could happen!

	nfound = 0;
	for (int i = 0; i < 3; i++)
	{
	    p = target.delta(vdx[i], vdy[i]);
	    if (target.mob()->canMove(p))
	    {
		// Yay!
		nfound++;
		if (!rand_choice(nfound))
		{
		    nt = p;
		    found = true;
		}
	    }
	}
	if (found)
	{
	    target = nt;
	    assert(!target.mob());
	    continue;		// But should exit!
	}

	// Try again, but find a mob to push.
	nfound = 0;
	for (int i = 0; i < 3; i++)
	{
	    p = target.delta(vdx[i], vdy[i]);
	    if (target.mob()->canMove(p, false))
	    {
		// Yay!
		nfound++;
		if (!rand_choice(nfound))
		{
		    nt = p;
		    found = true;
		}
	    }
	}
	if (!found)
	{
	    // OKay, this path failed to push.  Note that there might
	    // be other paths that *could* push.  We handle this
	    // by invoking this multiple times and having
	    // each invocation pick a random push path.
	    return false;
	}
	target = nt;
    }

    assert(!target.mob());
    // Now we should have a poslist chain to do pushing with.
    // We start with the end and move backwards.
    while (pushlist.entries())
    {
	MOB		*pushed;

	p = pushlist.pop();

	assert(!target.mob());

	// Move the mob from p to target.
	pushed = p.mob();
	if (pushed)
	{
	    pushed->setPos(target);
	    if (pushed->isDelayMob())
		pushed->setDelayMob(false, -1);
	    else
	    {
		// This is so no one can say that the kobolds are cheating!
		pushed->skipNextTurn();
	    }
	}

	// Now update target.
	target = p;
    }
    return true;
}

bool
mapPushMob(MOB *mob, int pdx, int pdy)
{
    // First, we only start a push if we are in one square
    // of the avatar.
    int			dx, dy;
    POS			avatarpos;

    // This shouldn't happen.
    if (!pdx && !pdy)
	return false;

    FORALL_8DIR(dx, dy)
    {
	if (mob->pos().delta(dx, dy).mob() &&
	    mob->pos().delta(dx, dy).mob()->isAvatar())
	{
	    avatarpos = mob->pos().delta(dx, dy);
	    break;
	}
    }
    // Refuse to push, no avatar nearby.
    if (!avatarpos.valid())
	return false;

    // Each push tries a differnt path, we pick 10 as a good test
    // to see if any of the paths are pushable.
    for (int i = 0; i < 10; i++)
    {
	if (mapRandomPushMob(mob, pdx, pdy))
	    return true;
    }
    return false;
}


void
mapReduceList(const MOBLIST &allmobs, MOBDELAYLIST &list)
{
    // Build our collision graph.
    int			i, j;
    MOBLIST		roots;

    MOBDELAYLIST	final;

    for (i = 0; i < allmobs.entries(); i++)
    {
	allmobs(i)->clearCollision();
	allmobs(i)->setDelayMob(false, -1);
    }

    for (i = 0; i < list.entries(); i++)
    {
	MOB		*mob = list(i).myMob;
	mob->setDelayMob(true, i);
    }

    // We alternately clear out all roots, then destroy a cycle,
    // until nothing is left.
    bool		mobstoprocess = true;
    while (mobstoprocess)
    {
	mapBuildContactGraph(list);

	// Find all roots.
	roots.clear();
	for (i = 0; i < list.entries(); i++)
	{
	    MOB		*mob = list(i).myMob;
	    if (!mob->alive())
		continue;
	    if (!mob->isDelayMob())
		continue;
	    if (!mob->collisionTarget() || !mob->collisionTarget()->isDelayMob())
		roots.append(mob);
	}

	// Note that as we append to roots, this will grow as we add more
	// entries.
	for (i = 0; i < roots.entries(); i++)
	{
	    MOB		*mob = roots(i);
	    bool	 	 resolved = false;

	    if (!mob->alive())
	    {
		resolved = true;
	    }
	    // Mobs may have lots theyr delay flag if they got pushed.
	    else if (mob->isDelayMob())
	    {
		int		 dx, dy, idx;
		MOB		*victim;

		idx = mob->delayMobIdx();
		dx = list(idx).myDx;
		dy = list(idx).myDy;

		victim = mob->pos().delta(dx, dy).mob();

		// If there is no collision target, easy!
		// Note that no collision target doesn't mean no victim!
		// Someone could have moved in the mean time or died.
		if (!victim)
		{
		    resolved = mob->actionWalk(dx, dy);
		}
		else
		{
		    // Try push?
		    resolved = mapPushMob(mob, dx, dy);
		}
	    }

	    if (resolved)
	    {
		mob->setDelayMob(false, -1);
		for (j = 0; j < mob->collisionSources().entries(); j++)
		{
		    // We moved out!
		    mob->collisionSources()(j)->setCollisionTarget(0);
		    roots.append(mob->collisionSources()(j));
		}
		mob->collisionSources().clear();
	    }
	    else
	    {
		// Crap, add to final.
		mapAddAllToFinal(mob, final, list);
	    }
	}

	// Rebuild our contact graph since various folks have moved or died.
	mapBuildContactGraph(list);

	// There are no roots, so either we have everything accounted for,
	// or there is a cycle.
	mobstoprocess = false;
	for (i = 0; i < list.entries(); i++)
	{
	    MOB		*mob = list(i).myMob;
	    if (!mob->alive())
		continue;
	    if (!mob->isDelayMob())
		continue;

	    // Finds a cycle, the returned mob is inside that cycle.
	    mob = mapFindCycle(mob);

	    if (mob)
	    {
		// Resolving this cycle may create new roots.
		mobstoprocess = true;
		mapResolveCycle(mob);
	    }
	}
    }

    // Only keep unprocessed delays.
    for (i = 0; i < list.entries(); i++)
    {
	MOB		*mob = list(i).myMob;
	if (mob->alive() && mob->isDelayMob())
	    final.append( list(i) );

	mob->setDelayMob(false, -1);
    }

    list = final;
}

void
MAP::doMoveNPC()
{
    MOBLIST		allmobs;
    int			i;
#ifdef DO_TIMING
    int 		movestart = TCOD_sys_elapsed_milli();
#endif

    // Note we don't need to pre-reap dead mobs since they won't
    // be in a room, so not be picked up in getAllMobs.

    // Pregather in case mobs move through portals.
    getAllMobs(allmobs);

    // The avatar may die during this process so can't test against him
    // in the inner loop.
    allmobs.removePtr(avatar());

    myDelayMobList.clear();
    myAllowDelays = true;

    // TODONE: A mob may kill another mob, causing this to die!
    // Our solution is just to check the alive flag.
    for (i = 0; i < allmobs.entries(); i++)
    {
	if (!allmobs(i)->alive())
	    continue;
	if (!allmobs(i)->aiForcedAction())
	    allmobs(i)->aiDoAI();
    }

#ifdef DO_TIMING
    int		movedoneai = TCOD_sys_elapsed_milli();
#endif

    int		delaysize = myDelayMobList.entries();
    int		prevdelay = delaysize+1;

    // So long as we are converging, it is good.
    while (delaysize && delaysize < prevdelay)
    {
	MOBDELAYLIST	list;

	// Copy the list as we will be re-registering everyone.
	list = myDelayMobList;

	// Randomizing the list helps avoid any problems from biased
	// input.
	// (A better approach would be to build a contact graph)
	list.shuffle();

	myDelayMobList.clear();

	mapReduceList(allmobs, list);

	for (i = 0; i < list.entries(); i++)
	{
	    // One way to reduce the list :>
	    if (!list(i).myMob->alive())
		continue;

	    // No need for force action test as it must have passed and
	    // no double jeapordy.
	    list(i).myMob->aiDoAI();
	}

	prevdelay = delaysize;

	delaysize = myDelayMobList.entries();
    }

    // Convergence!  Any left over mobs may still have another thing
    // they want to do, however, so we reinvoke with delay disabled.
    myAllowDelays = false;
    if (myDelayMobList.entries())
    {
	MOBDELAYLIST	list;

	// Copy the list as we will be re-registering everyone.
	list = myDelayMobList;

	// No need to randomize or anything.
	myDelayMobList.clear();

	for (i = 0; i < list.entries(); i++)
	{
	    // One way to reduce the list :>
	    if (!list(i).myMob->alive())
		continue;

	    // No need for force action test as it must have passed and
	    // no double jeapordy.
	    list(i).myMob->aiDoAI();
	}
    }

    // Delete any dead mobs
    reapMobs();

#ifdef DO_TIMING
    int		movedoneall = TCOD_sys_elapsed_milli();

    static int	movetime, posttime;

    movetime = movedoneai - movestart;
    posttime = movedoneall - movedoneai;

    cerr << "move time " << movetime << " post time " << posttime << endl;
    cerr << "Mob count " << getNumMobs() << endl;
#endif
}

MOB *
MAP::findAvatar()
{
    int			i, j;

    // Pregather in case mobs move through portals.
    for (i = 0; i < myRooms.entries(); i++)
    {
	for (j = 0; j < myRooms(i)->myMobs.entries(); j++)
	{
	    if (myRooms(i)->myMobs(j)->getDefinition() == MOB_AVATAR)
		return myRooms(i)->myMobs(j);
	}
    }
    return 0;
}

void
MAP::init()
{
    DIRECTORY		dir;
    const char		*f;
    BUF			buf;

    dir.opendir(SWORD_CFG_DIR "rooms");

    while (f = dir.readdir())
    {
	buf.strcpy(f);
	if (buf.extensionMatch("map"))
	{
	    ROOMTYPE_NAMES		roomtype;

	    FOREACH_ROOMTYPE(roomtype)
	    {
		if (buf.startsWith(glb_roomtypedefs[roomtype].prefix))
		{
		    BUF		fname;
		    fname.sprintf(SWORD_CFG_DIR "rooms/%s", f);
		    theFragRooms[roomtype].append(new FRAGMENT(fname.buffer()));
		}
	    }
#if 1
	    // Let all unmatched become generic rooms
	    if (roomtype == NUM_ROOMTYPES)
	    {
		buf.sprintf(SWORD_CFG_DIR "rooms/%s", f);
		theFragRooms[ROOMTYPE_NONE].append(new FRAGMENT(buf.buffer()));
	    }
#endif
	}
    }

    if (!theFragRooms[ROOMTYPE_NONE].entries())
    {
	cerr << "No .map files found in ../rooms" << endl;
	exit(-1);
    }
}

extern volatile int glbItemCache;

void
MAP::cacheItemPos()
{
    ITEMLIST		list;

#if 0
    getAllItems(list);

    for (int i = 0; i < list.entries(); i++)
    {
	glbItemCache = (int) (100 * (i / (float)list.entries()));
	buildDistMap(list(i)->pos());
    }

#endif
    glbItemCache = -1;
}

void
MAP::rebuildFOV()
{
    setAllFlags(MAPFLAG_FOV, false);
    setAllFlags(MAPFLAG_FOVCACHE, false);

    MOB_NAMES		mob;
    FOREACH_MOB(mob)
    {
	glbMobsInFOV[mob] = 0;
    }

    if (!avatar())
	return;

    delete myFOVCache;

    myFOVCache = new SCRPOS(avatar()->meditatePos(), 40, 25);
    POS			p;

    TCODMap		tcodmap(81, 51);
    int			x, y;

    for (y = 0; y < 51; y++)
    {
	for (x = 0; x < 81; x++)
	{
	    p = myFOVCache->lookup(x-40, y - 25);
	    p.setFlag(MAPFLAG_FOVCACHE, true);
	    // Force our own square to be visible.
	    if (x == 40 && y == 25)
	    {
		tcodmap.setProperties(x, y, true, true);
	    }
	    else if (ABS(x-40) < 2 && ABS(y-25) < 2)
	    {
		if (p.defn().semitransparent)
		    tcodmap.setProperties(x, y, true, true);
		else
		    tcodmap.setProperties(x, y, p.defn().istransparent, p.isPassable());
	    }
	    else
	    {
		tcodmap.setProperties(x, y, p.defn().istransparent, p.isPassable());
	    }
	}
    }

    tcodmap.computeFov(40, 25);

    for (y = 0; y < 51; y++)
    {
	for (x = 0; x < 81; x++)
	{
	    p = myFOVCache->lookup(x-40, y - 25);
	    // We might see this square in more than one way.
	    // We don't want to mark ourselves invisible if
	    // there is someway that did see us.
	    // This will result in too large an FOV, but the "free" squares
	    // are those that you could deduce from what you already 
	    // can see, so I hope to be benign?
	    if (tcodmap.isInFov(x, y))
	    {
		p.setFlag(MAPFLAG_FOV, true);
		if (p.mob())
		{
		    // This is slightly wrong as we could double
		    // count, but I'd argue that the rat seeing
		    // its portal rat honestly thinks it is an ally!
		    glbMobsInFOV[p.mob()->getDefinition()]++;
		}
	    }
	}
    }
}

bool
MAP::computeDelta(POS a, POS b, int &dx, int &dy)
{
    int		x1, y1, x2, y2;

    if (!myFOVCache)
	return false;

    // This early exits the linear search in myFOVCache find.
    if (!a.isFOVCache() || !b.isFOVCache())
	return false;

    if (!myFOVCache->find(a, x1, y1))
	return false;
    if (!myFOVCache->find(b, x2, y2))
	return false;

    dx = x2 - x1;
    dy = y2 - y1;

    POS		ap, at;

    // These are all in the coordinates of whoever built the scrpos,
    // ie, scrpos(0,0)
    //
    // This is a very confusing situation.
    // I think we have:
    // a - our source pos
    // @ - center of the FOV, myFOVCache->lookup(0,0)
    // a' - our source pos after walking from @, ie myFOVCache->lookup(x1,y1)
    //
    // dx, dy are in world space of @.  We want world space of a.
    // call foo.W the toWorld and foo.L the toLocal.
    // First apply the twist that @ got going to a'.
    // a'.W(@.L())
    // Next apply the transform into a.
    // a.W(a'.L())
    // Next we see a' cancels, leaving
    // a.W(@.L())
    //
    // Okay, that is bogus.  It is obvious it can't work as it
    // does not depend on a', and a' is the only way to include
    // the portal twist invoked in the walk!  I think I have the
    // twist backwards, ie, @.W(a'.L()), which means the compaction
    // doesn't occur.

    //
    // Try again.
    // @.L is needed to get into map space.
    // Then apply twist of @ -> a'   This is in Local coords!
    // So we just apply a.W
    // a.W(@.L(a'.W(@.L())))

    // Hmm.. 
    // ap encodes how to go from @ based world coords into
    // local a room map coords.  Thus ap.L gets us right there,
    // with only an a.W needed.

    ap = myFOVCache->lookup(x1, y1);
    // at = myFOVCache->lookup(0, 0);

    ap.rotateToLocal(dx, dy);
    a.rotateToWorld(dx, dy);

    return true;
}

void
MAP::buildReflexivePortal(POS pos, int dir)
{
    dir = (dir + 2);

    dir += pos.angle();

    dir %= 4;
    
    POS		backpos;

    backpos = myRooms(0)->findUserProtoPortal(dir);
    buildUserPortal(backpos, 1, dir, 0);
}

void
MAP::buildUserPortal(POS pos, int pnum, int dir, int avatardir)
{
    if (!pos.valid())
	return;

    if (!pnum)
    {
	buildReflexivePortal(pos, dir + avatardir);
    }

    if (myUserPortal[pnum].valid())
    {
	if (myUserPortal[0].valid() && myUserPortal[1].valid())
	{
	    // Both existing portals are wiped.
	    // Note the reversal of directins here!  This is because we
	    // have the coordinates of the landing zone and the portal
	    // only exists in the source room!
	    myUserPortal[1].room()->removePortal(myUserPortal[0]);
	    myUserPortal[0].room()->removePortal(myUserPortal[1]);
	}

	// Restore the wall, possibly trapping people :>
	myUserPortal[pnum].setTile(TILE_WALL);
	myUserPortal[pnum] = POS();
    }

    // Construct our new portal...
    pos.setTile(pnum ? TILE_ORANGEPORTAL : TILE_BLUEPORTAL);
    pos.setFlag(MAPFLAG_PORTAL, true);
    myUserPortal[pnum] = pos;
    myUserPortalDir[pnum] = dir;

    // Build the link
    if (myUserPortal[0].valid() && myUserPortal[1].valid())
    {
	ROOM::buildPortal(myUserPortal[0], myUserPortalDir[0],
			  myUserPortal[1], myUserPortalDir[1],
			    false);	
    }
}

MOB *
MAP::findMob(int uid) const
{
    for (int i = 0; i < myRooms.entries(); i++)
    {
	for (int j = 0; j < myRooms(i)->myMobs.entries(); j++)
	{
	    if (myRooms(i)->myMobs(j)->getUID() == uid)
	    {
		return myRooms(i)->myMobs(j);
	    }
	}
    }

    return 0;
}

POS
MAP::findRoomOfType(ROOMTYPE_NAMES roomtype)
{
    POS		result;
    int		nfound = 0;

    for (int i = 0; i < myRooms.entries(); i++)
    {
	if (myRooms(i)->type() == roomtype)
	{
	    nfound++;
	    if (!rand_choice(nfound))
	    {
		result = myRooms(i)->findCentralPos();
	    }
	}
    }
    return result;
}

void
MAP::save(ostream &os) const
{
    int32		val;
    int			i;
    u8			c;

    val = myAtlas;
    os.write((const char *) &val, sizeof(int32));

    ITEM::saveGlobal(os);

    for (i = 0; i < 2; i++)
    {
	myUserPortal[i].save(os);
	val = myUserPortalDir[i];
	os.write((const char *) &val, sizeof(int32));
    }

    val = myRooms.entries();
    os.write((const char *) &val, sizeof(int32));
    for (i = 0; i < myRooms.entries(); i++)
    {
	myRooms(i)->save(os);
    }

    ROOMTYPE_NAMES		roomtype;

    FOREACH_ROOMTYPE(roomtype)
    {
	val = glbRoomDiscovered[roomtype];
	os.write((const char *) &val, sizeof(int32));
	val = glbRoomLooted[roomtype];
	os.write((const char *) &val, sizeof(int32));
    }

    SPELL_NAMES		spell;
    FOREACH_SPELL(spell)
    {
	val = glbManaSpent[spell];
	os.write((const char *) &val, sizeof(int32));
    }

    val = glbCurrentQuest;
    os.write((const char *) &val, sizeof(int32));
    val = glbLastQuest;
    os.write((const char *) &val, sizeof(int32));
    val = glbWorldState;
    os.write((const char *) &val, sizeof(int32));
    val = glbLevel;
    os.write((const char *) &val, sizeof(int32));
}


MAP::MAP(istream &is)
{
    int32		val;
    int			i, n;

    myRefCnt.set(0);

    myUniqueId = glbMapId.add(1);
    myFOVCache = 0;
    myAllowDelays = false;

    for (i = 0; i < DISTMAP_CACHE; i++)
    {
	myDistMapWeight[i] = 0.0;
	myDistMapCache[i].setMap(this);
    }

    is.read((char *) &val, sizeof(int32));
    myAtlas = (ATLAS_NAMES) val;

    ITEM::loadGlobal(is);

    for (i = 0; i < 2; i++)
    {
	myUserPortal[i].load(is);
	myUserPortal[i].setMap(this);
	is.read((char *) &val, sizeof(int32));
	myUserPortalDir[i] = val;
    }

    is.read((char *) &val, sizeof(int32));
    n = val;
    for (i = 0; i < n; i++)
    {
	myRooms.append(ROOM::load(is));
	myRooms(i)->setMap(this);
    }

    ROOMTYPE_NAMES		roomtype;

    FOREACH_ROOMTYPE(roomtype)
    {
	is.read((char *) &val, sizeof(int32));
	glbRoomDiscovered[roomtype] = val ? true : false;
	is.read((char *) &val, sizeof(int32));
	glbRoomLooted[roomtype] = val ? true : false;
    }

    SPELL_NAMES		spell;
    FOREACH_SPELL(spell)
    {
	is.read((char *) &val, sizeof(int32));
	glbManaSpent[spell] = val;
    }

    is.read((char *) &val, sizeof(int32));
    glbCurrentQuest = (QUEST_NAMES) val;
    is.read((char *) &val, sizeof(int32));
    glbLastQuest = (QUEST_NAMES) val;
    is.read((char *) &val, sizeof(int32));
    glbWorldState = (WORLDSTATE_NAMES) val;
    is.read((char *) &val, sizeof(int32));
    glbLevel = val;

    myAvatar = findAvatar();
}
