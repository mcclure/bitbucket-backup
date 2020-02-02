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

#ifndef __map__
#define __map__

#include <iostream>
using namespace std;

#include "ptrlist.h"
#include "thread.h"
#include "glbdef.h"

class POS;
class ROOM;
class MOB;
class ITEM;
class MAP;
class FRAGMENT;
class DISPLAY;
class SCRPOS;
class MOBDELAY;

typedef PTRLIST<ROOM *> ROOMLIST;
typedef PTRLIST<MOB *> MOBLIST;
typedef PTRLIST<ITEM *> ITEMLIST;
typedef PTRLIST<POS> POSLIST;
typedef PTRLIST<FRAGMENT *> FRAGMENTLIST;
typedef PTRLIST<MOBDELAY> MOBDELAYLIST;

// Number of distance map caches we maintain.
#define DISTMAP_CACHE	16

extern bool	glbRoomDiscovered[NUM_ROOMTYPES];
extern bool	glbRoomLooted[NUM_ROOMTYPES];
extern int 	glbManaSpent[NUM_SPELLS];
extern QUEST_NAMES	glbCurrentQuest;
extern QUEST_NAMES	glbLastQuest;
extern WORLDSTATE_NAMES	glbWorldState;
extern int		glbLevel;
extern int	glbMobsInFOV[NUM_MOBS];

// The position is the only way to access elements of the map
// because it can adjust for orientation, etc.
// Functions that modify the underlying square are const if they
// leeave this unchanged...
class POS
{
public:
    POS();
    // Required for ptrlist, sigh.
    explicit POS(int blah);

    /// Equality ignores angles.
    bool 	operator==(const POS &cmp) const
    {	return cmp.myX == myX && cmp.myY == myY && cmp.myRoomId == myRoomId; }
    bool 	operator!=(const POS &cmp) const
    {	return cmp.myX != myX || cmp.myY != myY || cmp.myRoomId != myRoomId; }

    POS		left() const { return delta(-1, 0); }
    POS		up() const { return delta(0, -1); }
    POS		down() const { return delta(0, 1); }
    POS		right() const { return delta(1, 0); }

    int		angle() const { return myAngle; }

    POS		delta4Direction(int angle) const;
    POS		delta(int dx, int dy) const;

    POS		rotate(int angle) const;
    void	setAngle(int angle);

    bool	valid() const;

    /// Compute number of squares to get to goal, 8 way metric.
    /// This is an approximation, ignoring obstacles
    int		dist(POS goal) const;

    /// Search around ourself in growing circles tell we find a passable
    /// position.  Note that it may be limited to the room (see
    /// ROOM::spiralSearch)
    POS		spiralSearch(int startrad = 0, bool allowmob = false) const;

    ROOMTYPE_NAMES roomType() const;
    const ROOMTYPE_DEF &roomDefn() const { return glb_roomtypedefs[roomType()]; }

    /// Returns an estimate of what should be delta()d to this
    /// to move closer to goal.  Again, an estimate!  Traditionally
    /// just SIGN.
    void	dirTo(POS goal, int &dx, int &dy) const;

    /// Returns the first mob along the given vector from here.
    MOB		*traceBullet(int range, int dx, int dy, int *rangeleft=0) const;

    /// Returns the last valid pos before we hit a wall, if stop
    /// before wall set.  Otherwise returns the wall hit.
    POS		 traceBulletPos(int range, int dx, int dy, bool stopbeforewall, bool stopatmob = true) const;

    template <typename OP>
    void	 fireball(MOB *caster, int rad, u8 sym, ATTR_NAMES attr, OP op) const
    {
	// Boy do I hate C++ templates not supporting proper exports.
	int		dx, dy;

	POS		target;

	for (dy = -rad+1; dy <= rad-1; dy++)
	{
	    for (dx = -rad+1; dx <= rad-1; dx++)
	    {
		target = delta(dx, dy);
		if (target.valid())
		{
		    target.postEvent(EVENTTYPE_FORESYM, sym, attr);

		    // It is too hard to play if you harm yourself.
		    op(target);
		}
	    }
	}
    }

    /// Dumps to message description fo this square
    void	 describeSquare(bool blind) const;

    /// Draws the given bullet to the screen.
    void	 displayBullet(int range, int dx, int dy, u8 sym, ATTR_NAMES attr, bool stopatmob) const;

    void	 postEvent(EVENTTYPE_NAMES type, u8 sym, ATTR_NAMES attr) const;
    void	 postEvent(EVENTTYPE_NAMES type, u8 sym, ATTR_NAMES attr, const char *shout) const;

    /// Returns current distance map here.
    int		 getDistance(int mapnum) const;
    void	 setDistance(int mapnum, int dist) const;

    TILE_NAMES	tile() const;
    void	setTile(TILE_NAMES tile) const;
    MAPFLAG_NAMES		flag() const;
    void	setFlag(MAPFLAG_NAMES flag, bool state) const;

    /// prepares a square to be floored, ie, wall all surrounding
    /// and verify no portals.  Returns false if fails, note
    /// some walls may be created.
    bool	prepSquareForDestruction() const;

    /// Digs out this square
    /// Returns false if undiggable.  Might also fail if adjacent
    /// true squares aren't safe as we don't want to dig around portals,
    /// etc
    bool	digSquare() const;

    bool	isFOV() const { return flag() & MAPFLAG_FOV ? true : false; }
    bool	isFOVCache() const { return flag() & MAPFLAG_FOVCACHE ? true : false; }
    bool	isTrap() const { return flag() & MAPFLAG_TRAP ? true : false; }
    void	clearTrap() { setFlag(MAPFLAG_TRAP, false); }

    MOB 	*mob() const;
    ITEM 	*item() const;
    int	 	 allItems(ITEMLIST &items) const;

    MAP		*map() const { return myMap; }

    // Query functoins
    const TILE_DEF &defn() const { return defn(tile()); }
    static const TILE_DEF &defn(TILE_NAMES tile) { return glb_tiledefs[tile]; }

    bool	 isPassable() const { return defn().ispassable; }

    void	 setMap(MAP *map) { myMap = map; if (!map) myRoomId = -1; }

    void	 save(ostream &os) const;
    // In place load
    void	 load(istream &is);

    // A strange definition of const..
    void	 removeMob(MOB *mob) const;
    void	 addMob(MOB *mob) const;
    void	 removeItem(ITEM *item) const;
    // DO NOT ALL THIS, use move() on item.
    void	 addItem(ITEM *item) const;

    int 	 getAllMobs(MOBLIST &list) const;
    int 	 getAllItems(ITEMLIST &list) const;

    int		 getAllRoomItems(ITEMLIST &list) const;
    int		 getAllRoomMobs(MOBLIST &list) const;
    int		 getAllConnectedRoomMobs(MOBLIST &list) const;

    // Returns colour of the room we are in.  Do not stash this
    // pointer.
    u8		*color() const;
    int		*jacobian() const;

    // Use with much discretion!
    int		 roomId() const { return myRoomId; }

private:
    ROOM	*room() const;
    // Transforms dx & dy into our local facing.
    void	 rotateToLocal(int &dx, int &dy) const;
    // And back to world.
    void	 rotateToWorld(int &dx, int &dy) const;

    // Null if not on a map.
    int		 myRoomId;
    MAP		*myMap;

    // The room relative coords
    int			 myX, myY;
    // Which way we face in the room.
    int			 myAngle;

    friend class ROOM;
    friend class FRAGMENT;
    friend class MAP;
};

/// Note that portals are directed.  mySrc is assuemd to have angle of 0
/// as the turning effect is stored in myDst's angle.
class PORTAL
{
public:
    void	 save(ostream &os) const
    { mySrc.save(os);  myDst.save(os); }
    static PORTAL load(istream &is)
    {
	PORTAL		result;
	result.mySrc.load(is);
	result.myDst.load(is);
	return result;
    }

    POS		mySrc, myDst;
};

class ROOM
{
    ROOM();
    ROOM(const ROOM &room);
    ROOM &operator=(const ROOM &room);

    ~ROOM();

    int		width() const { return myWidth; }
    int		height() const { return myHeight; }

    static bool link(ROOM *a, int dira, ROOM *b, int dirb);
    static bool buildPortal(POS a, int dira, POS b, int dirb, bool settile=true);

    // mySrc is the portal square to launch from, myDst is the
    // destination square to land on
    POS		findProtoPortal(int dir);

    POS		findUserProtoPortal(int dir);

    ROOMTYPE_NAMES	type() const { return myType; }
    const ROOMTYPE_DEF &defn() const { return glb_roomtypedefs[type()]; }

    POS		findCentralPos() const;
    // Finds passable location near the start, but not nearer than
    // startrad.
    POS		spiralSearch(POS start, int startrad = 0, bool avoidmob=true) const;

    ATLAS_NAMES			 getAtlas() const;
    void	save(ostream &os) const;
    static ROOM	*load(istream &is);

private:
    // These all work in the room's local coordinates.
    TILE_NAMES		getTile(int x, int y) const;
    void		setTile(int x, int y, TILE_NAMES tile);
    MAPFLAG_NAMES	getFlag(int x, int y) const;

    void		setFlag(int x, int y, MAPFLAG_NAMES flag, bool state);
    void		setAllFlags(MAPFLAG_NAMES flag, bool state);

    int			getDistance(int mapnum, int x, int y) const;
    void		setDistance(int mapnum, int x, int y, int dist);
    void		clearDistMap(int mapnum);

    void		setColor(u8 r, u8 g, u8 b) 
			{ myColor[0] = r; myColor[1] = g; myColor[2] = b; }

    MOB			*getMob(int x, int y) const;
    void		 addMob(int x, int y, MOB *mob);
    void		 removeMob(MOB *mob);
    ITEM		*getItem(int x, int y) const;
    void		 addItem(int x, int y, ITEM *item);
    void		 removeItem(ITEM *item);
    void		 deleteAllItems();
    PORTAL		*getPortal(int x, int y) const;

    // Remvoes the portal that lands at dst.
    void		 removePortal(POS dst);

    // Removes all portals in this room
    void		 removeAllPortals();
    // Removes all portals out of this roomtype
    void		 removeAllForeignPortals();
    // Removes all portals heading into the given type.
    void		 removeAllPortalsTo(ROOMTYPE_NAMES type);

    void	 	 getAllMobs(int x, int y, MOBLIST &list) const;
    void	 	 getAllItems(int x, int y, ITEMLIST &list) const;

    void		 setMap(MAP *map);

    void		 resize(int w, int h);

    POS			 buildPos(int x, int y) const;
    POS			 getRandomPos(POS p, int *n) const;
    POS			 getRandomTile(TILE_NAMES tile, POS p, int *n) const;
    void		 findAllTiles(TILE_NAMES tile, POSLIST &list) const;

    void		 deleteContents();

    int			 getConnectedRooms(ROOMLIST &list) const;

    int			myId;
    MOBLIST		myMobs;
    ITEMLIST		myItems;
    PTRLIST<PORTAL>	myPortals;

    int			myWidth, myHeight;
    u8			*myTiles;
    u8			*myFlags;
    int			*myDists;
    MAP			*myMap;

    int			 myJacob[4];
    u8			 myColor[3];

    ROOMTYPE_NAMES	 myType;

    // Maybe null after load!
    const FRAGMENT	*myFragment;

    friend class POS;
    friend class FRAGMENT;
    friend class MAP;
};

// Stores the fact that a mob is being delayed and where it wants
// to go.
class MOBDELAY
{
public:
    MOBDELAY() { myMob = 0; myDx = 0; myDy = 0; }
    explicit MOBDELAY(int foo) { myMob = 0; myDx = 0; myDy = 0; }
    MOBDELAY(MOB *mob, int dx, int dy)
    { myMob = mob; myDx = dx; myDy = dy; }

    MOB		*myMob;
    int		 myDx, myDy;
};

class MAP
{
public:
    explicit MAP(ATLAS_NAMES atlas, MOB *avatar, DISPLAY *display);
    explicit MAP(istream &is);
    MAP(const MAP &map);
    MAP &operator=(const MAP &map);

    void			 incRef();
    void			 decRef();

    MOB				*avatar() const { return myAvatar; }
    void			 setAvatar(MOB *mob) { myAvatar = mob; }

    void			 doMoveNPC();

    int				 getNumMobs() const;
    int				 getAllMobs(MOBLIST &list) const;
    int				 getAllMobsOfType(MOBLIST &list, MOB_NAMES type) const;
    int				 getAllItems(ITEMLIST &list) const;

    void			 setAllFlags(MAPFLAG_NAMES flag, bool state);
    void			 killAllButAvatar();
    void			 lootAllValuable();

    int			 	 buildDistMap(POS center);

    // Adds a mob to our dead list.
    // This way we can delay actual desctruction to a safe time.
    void			 addDeadMob(MOB *mob);

    // Notes that this mob wants to move in this direction but
    // there is a mob in the way.  The collision resolution subsystem
    // should re-run the AI if it can clear out its destination.
    // If delays are disabled, returns false.  Otherwise true to
    // mark this ai complete.
    bool			 requestDelayedMove(MOB *mob, int dx, int dy);

    int				 getId() const { return myUniqueId; }
    ATLAS_NAMES			 getAtlas() const { return myAtlas; }

    int		getMobCount(MOB_NAMES mob) const { return myMobCount[mob]; }

    static void			 init();

    // Computes the delta to add to a to get to b.
    // Returns false if unable to compute it.  Things outside of FOV
    // are flakey as we don't cache them as I'm very, very, lazy.
    // And I already burned all the free cycles in the fire sim.
    bool			 computeDelta(POS a, POS b, int &dx, int &dy);

    // Reinitailizes FOV around avatar.
    void			 rebuildFOV();

    // Cache all the item position distances as we will likely
    // want them soon enough
    void			 cacheItemPos();

    void			 buildReflexivePortal(POS pos, int dir);
    void			 buildUserPortal(POS pos, int pnum, int dir,
						int avatardir);

    // Returns a central location in a random room of that type.
    POS				 findRoomOfType(ROOMTYPE_NAMES roomtype);

    // Finds matching mob from a uid.
    MOB				*findMob(int uid) const;

    POS			 	 getRandomTile(TILE_NAMES tile) const;
    // Finds all matching tiles, ACCUMULATING to poslist.
    void			 findAllTiles(TILE_NAMES tile, POSLIST &poslist) const;

    // Do not use.
    DISPLAY			*getDisplay() const { return myDisplay; }
    void			 setDisplay(DISPLAY *disp) { myDisplay = disp; }

    void			 save(ostream &os) const;

protected:
    ~MAP();

    int				allocDistMap(POS center);
    int				lookupDistMap(POS center) const;
    void			clearDistMap(int map);

    void			buildSquare(ROOMTYPE_NAMES roomtype,
				    ROOM *entrance, int enterdir,
				    ROOM *&exit, int &exitdir,
				    int w, int h);

    // Remove all dead mobs from our mob list.
    void			reapMobs();

    void			deleteContents();

    MOB				*findAvatar();

    ATOMIC_INT32		myRefCnt;

    int				myUniqueId;
    ATLAS_NAMES			myAtlas;

    POS				myDistMapCache[DISTMAP_CACHE];
    mutable double			myDistMapWeight[DISTMAP_CACHE];

    POS				myUserPortal[2];
    int				myUserPortalDir[2];

    ROOMLIST			myRooms;

    MOBLIST			myDeadMobs;

    MOB				*myAvatar;

    // The map as seen from our FOV.
    SCRPOS			*myFOVCache;

    DISPLAY			*myDisplay;

    bool			 myAllowDelays;
    MOBDELAYLIST		 myDelayMobList;
    int 			 myMobCount[NUM_MOBS];

    static FRAGMENTLIST		 theFragRooms[NUM_ROOMTYPES];

    friend class FRAGMENT;
    friend class ROOM;
    friend class POS;
};


#endif
