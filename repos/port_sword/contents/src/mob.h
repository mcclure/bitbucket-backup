/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        mob.h ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#ifndef __mob__
#define __mob__

#include "glbdef.h"

#include "grammar.h"
#include "dpdf.h"
#include "ptrlist.h"
#include "map.h"

#include <iostream>
using namespace std;

class MAP;
class ITEM;

// These states are the Strategy layer.
enum STRATEGY_STATE
{
    STRATEGY_SAMEROOM,		// The default.
    STRATEGY_FETCHITEMS,	// Take everything to the store room
    STRATEGY_AMBUSH,		// Wait in ambush
    STRATEGY_KILL,		// Eliminate the threat in the ambush room
    STRATEGY_KILLCHASE,		// Eliminate the threat
    STRATEGY_PARTY,		// All is good again.
};

class MOB
{
public:
    static MOB		*create(MOB_NAMES def);

    // This can return 0 if no valid mobs at that depth!
    static MOB		*createNPC(ATLAS_NAMES atlas);

    // Creates a copy.  Ideally we make this copy on write so
    // is cheap, but mobs probably move every frame anyways.
    MOB			*copy() const;
    
    ~MOB();

    const POS		&pos() const { return myPos; }

    bool		 isMeditating() const
			 { return myMeditatePos.valid(); }

    void		 meditateMove(POS dest);

    POS			 meditatePos() const
			 { if (isMeditating()) return myMeditatePos;
			   return pos();
			 }
    void		 setPos(POS pos);
    void		 setMap(MAP *map);
    void		 clearAllPos();

    bool		 alive() const { return myHP > 0; }

    int			 getHP() const { return myHP; }
    int			 getMaxHP() const { return defn().max_hp; }
    int			 getMP() const { return myMP; }
    int			 getMaxMP() const { return defn().max_mp; }
    bool		 isFullHealth() const { return myHP == getMaxHP(); }

    void		 gainHP(int hp);
    // hp is positive to lose, do not use this to kill!
    void		 loseHP(int hp);
    void		 gainMP(int mp);
    void		 corrupt(int corruption);

    int			 numDeaths() const { return myNumDeaths; }

    void		 setHome(POS pos) { myHome = pos; }

    bool		 isSwallowed() const { return myIsSwallowed; }
    void		 setSwallowed(bool isswallowed) { myIsSwallowed = isswallowed; }

    // Returns true if we die!
    bool		 applyDamage(MOB *src, int hp, ELEMENT_NAMES element,
				    ATTACKSTYLE_NAMES attackstyle);
    // The paper work of killing.
    void		 kill();

    // posts an event tied to this mobs location.
    void		 postEvent(EVENTTYPE_NAMES type, u8 sym, ATTR_NAMES attr, const char *text = 0) const;
    void		 postEvent(EVENTTYPE_NAMES type, u8 sym, ATTR_NAMES attr, BUF buffer) const { postEvent(type, sym, attr, buffer.buffer()); }


    MOB_NAMES		 getDefinition() const { return myDefinition; }

    const MOB_DEF	&defn() const { return defn(getDefinition()); }
    static const MOB_DEF &defn(MOB_NAMES mob) { return glb_mobdefs[mob]; }

    bool		 isAvatar() const { return this == getAvatar(); }
    MOB			*getAvatar() const { if (pos().map()) return pos().map()->avatar(); return 0; }

    VERB_PERSON		 getPerson() const;

    // Symbol and attributes for rendering.
    void		 getLook(u8 &symbol, ATTR_NAMES &attr) const;

    // Retrieve the raw name.
    const char		*getName() const;
    const char		*getRawName() const;

    BUF			 getLongDescription() const;

    // Returns true if we have the item in question.
    bool		 hasItem(ITEM_NAMES itemname) const;
    // Gives this mob a new version of the item, useful for flags.
    // Returns the resulting item.
    ITEM		*giftItem(ITEM_NAMES itemname);
    ITEM 		*lookupItem(ITEM_NAMES itemname) const;
    ITEM 		*lookupWeapon() const;
    ITEM		*lookupArmour() const;
    ITEM 		*lookupWand() const;
    ITEM 		*lookupRing() const;
    RING_NAMES		 lookupRingName() const;
    ITEM		*getRandomItem() const;

    int			 getMeleeDamage() const;
    const char		*getMeleeVerb() const;
    const char		*getMeleeWeaponName() const;
    DPDF		 getMeleeDPDF() const;
    ELEMENT_NAMES	 getMeleeElement() const;
    int			 getRangedDamage() const;
    DPDF		 getRangedDPDF() const;
    int			 getRangedRange() const;
    const char		*getRangedWeaponName() const;
    ELEMENT_NAMES	 getRangedElement() const;

    void		 getRangedLook(u8 &symbol, ATTR_NAMES &attr) const;

    bool		 canMoveDir(int dx, int dy, bool checkmob = true) const;
    bool		 canMove(POS pos, bool checkmob = true) const;

    // true if we don't feel like going there.
    bool		 aiAvoidDirection(int dx, int dy) const;

    void		 move(POS newpos, bool ignoreangle = false);

    bool		 isFriends(const MOB *other) const;
    // Number of hostiles surrounding this.
    int			 numberMeleeHostiles() const;
    // True if we can hit that square with our current ranged weapon.
    bool		 canTargetAtRange(POS goal) const;

    void		 setSearchPower(int power) { mySearchPower = power; }

    void		 addItem(ITEM *item);
    void		 removeItem(ITEM *item, bool quiet = false);
    void		 updateEquippedItems();
    // Decreases the items count by one, returning a singular copy
    // of the item the caller must delete.  (But is useful for getting
    // a single verb)
    ITEM		*splitStack(ITEM *item);

    void		 loseTempItems();
    void		 loseAllItems();

    void		 clearBoredom() { myBoredom = 0; }

    // Message reporting, only spams if the player can see it.
    void		 formatAndReport(const char *msg);
    void		 formatAndReport(BUF buf) { formatAndReport(buf.buffer()); }
    void		 formatAndReport(const char *msg, MOB *object);
    void		 formatAndReport(BUF buf, MOB *object) { formatAndReport(buf.buffer(), object); }
    void		 formatAndReport(const char *msg, ITEM *object);
    void		 formatAndReport(BUF buf, ITEM *object) { formatAndReport(buf.buffer(), object); }
    void		 formatAndReport(const char *msg, const char *object);
    void		 formatAndReport(BUF buf, const char *object) { formatAndReport(buf.buffer(), object); }
    void		 formatAndReport(const char *msg, BUF object) { formatAndReport(msg, object.buffer()); }
    void		 formatAndReport(BUF buf, BUF object) { formatAndReport(buf.buffer(), object.buffer()); }


    //
    // High level AI functions.
    //

    AI_NAMES		 getAI() const;

    // Determines if there are any mandatory actions to be done.
    // Return true if a forced action occured, in which case the
    // player gets no further input.
    bool		 aiForcedAction();

    void		 aiTrySpeaking();

    // Runs the normal AI routines.  Called if aiForcedAction failed
    // and not the avatar.
    bool		 aiDoAI();
    // Allows you to force a specific ai type.
    bool		 aiDoAIType(AI_NAMES aitype);

    // Runs the twitch AI.  Stuff that we must do the next turn.
    bool		 aiTwitch(MOB *avatar);

    // Runs the tatics AI.  How to fight this battle.
    bool		 aiTactics(MOB *avatar);

    // Runs the stratgey based AI
    bool		 aiStrategy();

    // Before we wade into battle
    bool		 aiBattlePrep();

    // Deny the avatar!
    bool		 aiDestroySomethingGood(MOB *denyee);

    // Updates our lock on the avatar - tracking if we know
    // where he is.  Position of last known location is in myTX.
    bool		 aiAcquireAvatar();
    bool		 aiAcquireTarget(MOB *foe);

    // Determine which item we like more.
    ITEM		*aiLikeMoreWeapon(ITEM *a, ITEM *b) const;
    ITEM		*aiLikeMoreWand(ITEM *a, ITEM *b) const;
    ITEM		*aiLikeMoreArmour(ITEM *a, ITEM *b) const;

    // Hugs the right hand walls.
    bool		 aiDoMouse();
    // Only attacks in numbers
    bool		 aiDoRat();

    // Runs in straight lines until it hits something.
    bool		 aiStraightLine();

    // Charges the listed mob if in FOV
    bool		 aiCharge(MOB *foe, AI_NAMES aitype, bool orthoonly = false);

    // Can find mob anywhere on the mob - charges and kills.
    bool		 aiKillPathTo(MOB *target);

    // Attempts a ranged attack against the avatar
    bool		 aiRangeAttack(MOB *target = 0);

    // Runs away from (x, y).  Return true if action taken.
    bool		 aiFleeFrom(POS goal, bool sameroom = false);
    bool		 aiFleeFromAvatar();
    // Makes the requirement the new square is not adjacent.
    bool		 aiFleeFromSafe(POS goal, bool avoidrange, bool avoidmob);
    bool		 aiFleeFromSafe(POS goal, bool avoidrange);

    // Runs straight towards (x, y).  Return true if action taken.
    bool		 aiMoveTo(POS goal, bool orthoonly = false);

    // Does a path find to get to x/y.  Returns false if blocked
    // or already at x & y.
    bool		 aiPathFindTo(POS goal);
    bool		 aiPathFindTo(POS goal, bool avoidmob);
    // Does a path find, trying to avoid the given mob if not 0
    bool		 aiPathFindToAvoid(POS goal, MOB *avoid);
    bool		 aiPathFindToAvoid(POS goal, MOB *avoid, bool avoidmob);

    // Tries to go to (x, y) by flanking them.
    bool		 aiFlankTo(POS goal);
    
    bool		 aiRandomWalk(bool orthoonly = false, bool sameroom = false);

    // Action methods.  These are how the AI and user manipulates
    // mobs.
    // Return true if the action consumed a turn, else false.
    bool		 actionBump(int dx, int dy);
    bool		 actionRotate(int angle);
    bool		 actionChat(int dx, int dy);
    bool		 actionMelee(int dx, int dy);
    bool		 actionWalk(int dx, int dy);
    bool		 actionFire(int dx, int dy);
    bool		 actionMagicMove(MAGICMOVE_NAMES move, int dx, int dy);
    bool		 actionCast(SPELL_NAMES spell, int dx, int dy);
    bool		 actionThrow(ITEM *item, int dx, int dy);
    bool		 actionShop(SHOP_NAMES shop, int goldcost, int parm);
    bool		 actionPortalFire(int dx, int dy, int portal);
    bool		 actionPickup();
    bool		 actionPickup(ITEM *item);
    bool		 actionDrop(ITEM *item);
    bool		 actionDropButOne(ITEM *item);
    bool		 actionDropSurplus();
    bool		 actionEat(ITEM *item);
    bool		 actionQuaff(ITEM *item);
    bool		 actionBreak(ITEM *item);
    bool		 actionWear(ITEM *item);
    bool		 actionMeditate();
    bool		 actionSearch(bool silent = false);

    bool		 actionYell(YELL_NAMES yell);

    bool		 actionClimb();

    void		 save(ostream &os) const;
    static MOB		*load(istream &is);

    const ITEMLIST	&inventory() const { return myInventory; }
    ITEM		*getItemFromNo(int itemno) const
			{ if (itemno < 0 || itemno >= myInventory.entries()) return 0; return myInventory(itemno); }

    void		 getVisibleEnemies(PTRLIST<MOB *> &list) const;
    bool		 hasVisibleEnemies() const;
    bool		 hasSurplusItems() const;
    int			 numSurplusRange() const;
    int			 hasDestroyables() const;

    bool		 aiWantsItem(ITEM *item) const;
    bool		 aiWantsAnyMoreItems() const;

    bool		 buildPortalAtLocation(POS vpos, int portal) const;

    // Searches the given square.
    void		 searchOffset(int dx, int dy, bool silent);

    template <typename OP>
    bool		 doRangedAttack(int range, int area, int dx, int dy,
				u8 sym, ATTR_NAMES attr,
				const char *verb, OP op);
    void		 triggerManaUse(SPELL_NAMES spell, int manaused);

    void		 clearCollision();
    MOBLIST		&collisionSources() { return myCollisionSources; }
    const MOBLIST	&collisionSources() const { return myCollisionSources; }
    MOB			*collisionTarget() const { return myCollisionTarget; }
    // Handles sources backpointer.
    void		 setCollisionTarget(MOB *target);

    bool	  	 isDelayMob() const { return myDelayMob; }
    int			 delayMobIdx() const { return myDelayMobIdx; }
    void		 setDelayMob(bool val, int idx) 
			 { myDelayMob = val;
				myDelayMobIdx = idx; }
    
    int			 getUID() const { return myUID; }

    void		 reportSquare(POS p);

    void		 skipNextTurn() { mySkipNextTurn = true; }
    
protected:
    MOB();

    MOB_NAMES		 myDefinition;

    POS			 myPos;
    POS			 myMeditatePos;

    int			 myNumDeaths;

    ITEMLIST		 myInventory;

    // Current target
    POS			 myTarget;
    int			 myFleeCount;
    int			 myBoredom;
    int			 myYellHystersis;

    bool		 myIsSwallowed;

    bool		 myHeardYell[NUM_YELLS];
    bool		 myHeardYellSameRoom[NUM_YELLS];
    bool		 mySawMurder;
    bool		 mySawMeanMurder;
    bool		 mySawVictory;
    bool		 myAvatarHasRanged;

    // State machine
    int			 myAIState;

    // My home spot.
    POS			 myHome;

    // Hitpoints
    int			 myHP;
    int			 myMP;

    int			 mySearchPower;

    // Useless ID
    int			 myUID;

    MOB			*myCollisionTarget;
    MOBLIST		 myCollisionSources;

    bool		 mySkipNextTurn;
    bool		 myDelayMob;
    int			 myDelayMobIdx;

    STRATEGY_STATE	myStrategy;
};

#endif

