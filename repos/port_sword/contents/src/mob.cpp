/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        mob.cpp ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#include "mob.h"

#include "map.h"
#include "msg.h"
#include "text.h"
#include "speed.h"
#include "item.h"
#include "event.h"
#include "display.h"
#include "engine.h"

#include <stdio.h>

#include <iostream>
using namespace std;

//
// Fireball operators
//
class ATTACK_OP
{
public:
    ATTACK_OP(MOB *src, DPDF dpdf, ELEMENT_NAMES elem)
    {
	mySrc = src;
	myDpdf = dpdf;
	myElement = elem;
    }

    void operator()(POS p)
    {
	if (p.mob() && p.mob() != mySrc)
	    p.mob()->applyDamage(mySrc, myDpdf.evaluate(), myElement,
				    ATTACKSTYLE_RANGE);
    }

private:
    DPDF		 myDpdf;
    ELEMENT_NAMES	 myElement;
    MOB			*mySrc;
};

class POTION_OP
{
public:
    POTION_OP(MOB *src, POTION_NAMES potion, bool *interest) 
    { mySrc = src; myPotion = potion; myInterest = interest; }

    void operator()(POS p)
    {
	MOB		*mob = p.mob();
	switch (myPotion)
	{
	    case POTION_HEAL:
		if (mob && !mob->isFullHealth())
		{
		    *myInterest = true;
		    mob->formatAndReport("%S <look> healthier.");
		    mob->gainHP(15);
		}
		break;
	    case POTION_SPEED:
		if (mob && !mob->hasItem(ITEM_QUICKBOOST))
		{
		    *myInterest = true;
		    mob->giftItem(ITEM_QUICKBOOST);
		}
		break;
	    case POTION_CURE:
		if (mob && mob->hasItem(ITEM_POISON))
		{
		    ITEM	*p = mob->lookupItem(ITEM_POISON);
		    mob->removeItem(p);
		    *myInterest = true;
		}
		break;

	    case POTION_POISON:
		if (mob && !mob->hasItem(ITEM_POISON))
		    *myInterest = true;
		if (mob)
		{
		    mob->giftItem(ITEM_POISON);
		    ITEM	*p = mob->lookupItem(ITEM_POISON);
		    if (p)
			p->addTimer(10);
		    // Token damage to the mob to ensure we get blamed
		    mob->applyDamage(mySrc, 0, ELEMENT_POISON,
				    ATTACKSTYLE_INTERNAL);
		}
		break;

	    case POTION_JUICE:
		break;

	    case POTION_ACID:
		if (mob)
		{
		    *myInterest = true;
		    mob->formatAndReport("%S <be> burned by the acid!");
		    mob->applyDamage(mySrc, 10, ELEMENT_ACID, 
					ATTACKSTYLE_RANGE);
		}
		break;
	}
    }

private:
    bool		*myInterest;
    POTION_NAMES	 myPotion;
    MOB			*mySrc;
};


//
// MOB Implementation
//

MOB::MOB()
{
    myFleeCount = 0;
    myBoredom = 0;
    myYellHystersis = 0;
    myAIState = 0;
    myHP = 0;
    myIsSwallowed = false;
    myNumDeaths = 0;
    myUID = INVALID_UID;
    myStrategy = STRATEGY_SAMEROOM;
    mySkipNextTurn = false;
    myDelayMob = false;
    myDelayMobIdx = -1;
    myCollisionTarget = 0;
    mySearchPower = 0;

    YELL_NAMES	yell;
    FOREACH_YELL(yell)
    {
	myHeardYell[yell] = false;
	myHeardYellSameRoom[yell] = false;
    }
    mySawMurder = false;
    mySawMeanMurder = false;
    mySawVictory = false;
    myAvatarHasRanged = false;
}

MOB::~MOB()
{
    int		i;

    if (myPos.map() && myPos.map()->avatar() == this)
	myPos.map()->setAvatar(0);

    myPos.removeMob(this);

    for (i = 0; i < myInventory.entries(); i++)
	delete myInventory(i);
}

MOB *
MOB::create(MOB_NAMES def)
{
    MOB		*mob;

    mob = new MOB();

    mob->myDefinition = def;

    mob->myHP = glb_mobdefs[def].max_hp;
    mob->myMP = glb_mobdefs[def].max_mp;

    mob->myUID = glb_allocUID();

    if (def == MOB_AVATAR)
    {
	ITEM		*i;
	i = ITEM::create(ITEM_FREYBLADE);
	// i = ITEM::create(ITEM_CRYSTALSWORD);
	i->setBroken(false);
	mob->addItem(i);

	i = ITEM::create(ITEM_CLOTHES);
	i->setBroken(false);
	mob->addItem(i);

	// Avatar starts with no mana.
	mob->myMP = 0;

	// We start at low health.
	mob->myHP = 35;
    }

    return mob;
}

MOB *
MOB::copy() const
{
    MOB		*mob;

    mob = new MOB();
    
    *mob = *this;

    // Copy inventory one item at a time.
    // We are careful to maintain the same list order here so restoring
    // won't shuffle things unexpectadly
    int		 i;

    mob->myInventory.clear();
    for (i = 0; i < myInventory.entries(); i++)
    {
	mob->myInventory.append(myInventory(i)->copy());
    }
    
    return mob;
}

void
MOB::setPos(POS pos)
{
    myPos.removeMob(this);
    myPos = pos;
    myPos.addMob(this);
}


void
MOB::setMap(MAP *map)
{
    myPos.setMap(map);
    myTarget.setMap(map);
    myHome.setMap(map);
    myMeditatePos.setMap(map);
}

void
MOB::clearAllPos()
{
    myPos = POS();
    myTarget = POS();
    myHome = POS();
    myMeditatePos = POS();
}

MOB *
MOB::createNPC(ATLAS_NAMES atlas)
{
    MOB_NAMES	mob = MOB_NONE, testmob;
    int		choice = 0;
    MOB		*m;

    // Given the letter choice, choose a mob that matches it appropriately.
    choice = 0;

    for (const char *mobid = glb_atlasdefs[atlas].mobs;
	 *mobid;
	 mobid++)
    {
	testmob = (MOB_NAMES) *mobid;
	// Stuff with 0 depth is never created automatically.
	if (!glb_mobdefs[testmob].depth)
	    continue;

	// Use the baseletter to bias the creation.
	// if (glb_mobdefs[testmob].depth <= depth)
	{
	    if (rand_choice(choice + glb_mobdefs[testmob].rarity) < glb_mobdefs[testmob].rarity)
		mob = testmob;
	    choice += glb_mobdefs[testmob].rarity;
	}
    }

    if (mob == MOB_NONE)
	return 0;

    // Testing..
    // mob = MOB_PYTHON;

    m = MOB::create(mob);

    if (0)
    {
	ITEM *item = ITEM::createRandom(atlas);
	if (!rand_choice(5))
	    item = ITEM::create(ITEM_BREAD);
	if (item)
	    m->addItem(item);
    }

    return m;
}

void
MOB::getLook(u8 &symbol, ATTR_NAMES &attr) const
{
    symbol = glb_mobdefs[getDefinition()].symbol;
    attr = (ATTR_NAMES) glb_mobdefs[getDefinition()].attr;

    if (!alive())
    {
	// Dead creatures override their attribute to blood
	attr = ATTR_RED;
    }
}

const char *
MOB::getName() const
{
    if (isAvatar())
	return "you";

    return glb_mobdefs[getDefinition()].name;
}

const char *
MOB::getRawName() const
{
    if (isAvatar())
	return glb_mobdefs[getDefinition()].name;

    return getName();
}

BUF
MOB::getLongDescription() const
{
    BUF		descr, detail;
    BUF		stats;

    descr.strcpy(gram_capitalize(getRawName()));
    descr.append('\n');
    descr.append('\n');

    int		min, q1, q2, q3, max;

    stats.sprintf("Health: %d/%d\n", getHP(), getMaxHP());
    descr.strcat(stats);

    ITEM_NAMES		i;
    FOREACH_ITEM(i)
    {
	if (glb_itemdefs[i].isflag)
	{
	    if (hasItem(i))
	    {
		ITEM		*item = lookupItem(i);
		int		timer = item ? item->getTimer() : -1;

		if (timer >= 0)
		{
		    stats.sprintf("%s for %d more turns\n", 
				    glb_itemdefs[i].name, 
				    timer);
		}
		else
		    stats.sprintf("%s permamently\n", 
				    glb_itemdefs[i].name);
		descr.strcat(stats);
	    }
	}
    }

    getMeleeDPDF().getQuartile(min, q1, q2, q3, max);
    BUF		weaponname = gram_capitalize(getMeleeWeaponName());
    if (min == max)
	stats.sprintf("Weapon: %s\n        Damage: %d\n",
		    weaponname.buffer(),
		    max);
    else
	stats.sprintf("Melee Weapon: %s (%d..%d..%d..%d..%d)\n",
		    weaponname.buffer(),
		    min, q1, q2, q3, max);
    descr.strcat(stats);

    if (defn().range_valid)
    {
	getRangedDPDF().getQuartile(min, q1, q2, q3, max);
	BUF		rangename = gram_capitalize(getRangedWeaponName());
	if (min != max)
	    stats.sprintf("Ranged Weapon: %s (%d..%d..%d..%d..%d), Range %d\n",
			rangename.buffer(),
			min, q1, q2, q3, max,
			getRangedRange());
	else
	    stats.sprintf("Ranged: %s\n        Damage: %d\n        Range %d\n",
			rangename.buffer(),
			max,
			getRangedRange());
	descr.strcat(stats);
    }
    descr.append('\n');

    // Dump the text.txt if present.
    detail = text_lookup("mob", getRawName());
    if (detail.isstring() && !detail.startsWith("Missing text entry: "))
    {
	descr.strcat(detail);
    }

    return descr;
}

ITEM *
MOB::getRandomItem() const
{
    ITEM	*result;
    int		choice = 0;

    result = 0;

    if (myInventory.entries())
    {
	choice = rand_choice(myInventory.entries());
	return myInventory(choice);
    }
    return result;
}

bool
MOB::hasItem(ITEM_NAMES itemname) const
{
    if (lookupItem(itemname))
	return true;

    return false;
}

ITEM *
MOB::giftItem(ITEM_NAMES itemname)
{
    ITEM		*item;

    item = ITEM::create(itemname);

    addItem(item);

    // Allow for stacking!
    item = lookupItem(itemname);

    return item;
}

ITEM *
MOB::lookupWeapon() const
{
    ITEM 	*bestweapon = 0;
    for (int i = 0; i < myInventory.entries(); i++)
    {
	if (myInventory(i)->isMelee() && !myInventory(i)->isBroken())
	    bestweapon = aiLikeMoreWeapon(myInventory(i), bestweapon);
    }
    return bestweapon;
}

ITEM *
MOB::lookupArmour() const
{
    ITEM 	*bestarmour = 0;
    for (int i = 0; i < myInventory.entries(); i++)
    {
	if (myInventory(i)->isArmour() && !myInventory(i)->isBroken())
	    bestarmour = aiLikeMoreArmour(myInventory(i), bestarmour);
    }
    return bestarmour;
}

ITEM *
MOB::lookupWand() const
{
    ITEM 	*bestwand = 0;
    for (int i = 0; i < myInventory.entries(); i++)
    {
	if (myInventory(i)->isRanged() && !myInventory(i)->isBroken())
	    bestwand = aiLikeMoreWand(myInventory(i), bestwand);
    }
    return bestwand;
}

ITEM *
MOB::lookupItem(ITEM_NAMES itemname) const
{
    int		i;

    for (i = 0; i < myInventory.entries(); i++)
    {
	if (myInventory(i)->getDefinition() == itemname)
	    return myInventory(i);
    }
    return 0;
}

ITEM *
MOB::lookupRing() const
{
    for (int i = 0; i < myInventory.entries(); i++)
    {
	if (myInventory(i)->isRing() && myInventory(i)->isEquipped())
	    return myInventory(i);
    }
    return 0;
}

RING_NAMES
MOB::lookupRingName() const
{
    ITEM		*ring = lookupRing();
    if (!ring)
	return RING_NONE;

    return (RING_NAMES) ring->getMagicClass();
}

bool
MOB::canMoveDir(int dx, int dy, bool checkmob) const
{
    POS		goal;

    goal = myPos.delta(dx, dy);

    return canMove(goal, checkmob);
}

bool
MOB::canMove(POS pos, bool checkmob) const
{
    if (!pos.valid())
	return false;

    if (!pos.isPassable())
    {
	if (pos.defn().isphaseable && defn().passwall)
	{
	    // Can move through it...
	}
	else if (pos.defn().isdiggable && defn().candig)
	{
	    // Could dig through it.
	}
	else
	{
	    // Failed...
	    return false;
	}
    }

    if (checkmob && pos.mob())
	return false;

    return true;
}

void
MOB::move(POS newpos, bool ignoreangle)
{
    // If we have swallowed something, move it too.
    if (!isSwallowed())
    {
	PTRLIST<MOB *>	moblist;
	int		i;

	pos().getAllMobs(moblist);
	for (i = 0; i < moblist.entries(); i++)
	{
	    if (moblist(i)->isSwallowed())
	    {
		moblist(i)->move(newpos, true);
	    }
	}
    }

    if (ignoreangle)
	newpos.setAngle(pos().angle());

    setPos(newpos);

    reportSquare(pos());
}

void
MOB::gainHP(int hp)
{
    int		maxhp, netnew;

    maxhp = defn().max_hp;

    netnew = myHP;
    myHP += hp;
    if (myHP > maxhp)
	myHP = maxhp;
    netnew = myHP - netnew;
    if (isAvatar() && pos().map())
    {
	pos().map()->getDisplay()->postHealthChange(netnew);
    }
}

void
MOB::loseHP(int hp)
{
    int		netnew;

    netnew = myHP;
    myHP -= hp;
    if (myHP < 0)
	myHP = 0;
    netnew = myHP - netnew;
    if (isAvatar() && pos().map())
    {
	pos().map()->getDisplay()->postHealthChange(netnew);
    }
}

void
MOB::gainMP(int mp)
{
    int		maxmp;

    maxmp = defn().max_mp;

    myMP += mp;
    if (myMP > maxmp)
	myMP = maxmp;
}

void
MOB::postEvent(EVENTTYPE_NAMES type, u8 sym, ATTR_NAMES attr, const char *text) const
{
    if (pos().map())
	pos().map()->getDisplay()->queue().append(EVENT((MOB *)this, sym, attr, type, text));
}


bool
MOB::applyDamage(MOB *src, int hits, ELEMENT_NAMES element, ATTACKSTYLE_NAMES attackstyle)
{
    // Being hit isn't boring.
    clearBoredom();

    if (hasItem(ITEM_INVULNERABLE))
	return false;

    if (element == ELEMENT_LIGHT && hasItem(ITEM_BLIND))
    {
	return false;
    }

    if (element == ELEMENT_ACID && getDefinition() == MOB_SLIME)
    {
	// Slimes don't mind acid!
	return false;
    }

    // Adjust damage down for armour.
    int			 ac = 0;

    if (lookupArmour())
	ac += lookupArmour()->getAC();

    if (attackstyle == ATTACKSTYLE_INTERNAL)
	ac = 0;

    // Add elemental resist.
    RING_NAMES		ring;

    ring = lookupRingName();
    if (ring != RING_NONE)
    {
	if (element == glb_ringdefs[ring].resist)
	    ac += glb_ringdefs[ring].resist_amt;

	if (attackstyle == ATTACKSTYLE_RANGE)
	    ac += glb_ringdefs[ring].deflect;
    }

    // Worse case is 100x damage.  A pretty damn worse case!
    if (ac <= -99)
	ac = -99;

    if (ac)
    {
	float		ratio = 100.0F / (100 + ac);
	float		fhits = hits * ratio;

	hits = int(fhits);
	fhits -= hits;

	// Add the round off error.
	if (rand_double() < fhits)
	    hits++;
    }

    if (hits >= getHP())
    {
	// Ensure we are dead to alive()
	myHP = 0;
	myNumDeaths++;
	// Rather mean.
	myMP = 0;

	// Death!
	if (src)
	{
	    if (src->isAvatar() && !isAvatar())
	    {
		// Drain message.
		if (defn().isundead)
		    src->formatAndReport("%S <kill> %O and <drain> its unlife!", this);
		else
		    src->formatAndReport("%S <kill> %O and <drain> its life!", this);
	    }
	    else
	    {
		src->formatAndReport("%S <kill> %O!", this);
	    }
	}
	else
	    formatAndReport("%S <be> killed!");

	// If there is a source, and they are swallowed,
	// they are released.
	if (src && src->isSwallowed())
	{
	    src->setSwallowed(false);
	}
	
	// If we are the avatar, special stuff happens.
	if (isAvatar())
	{
	    // never update on this thread!
	    // msg_update();
	    // TODO: Pause something here?
	}
	else
	{
	    // Drop any stuff we have.
	    int i;
	    for (i = 0; i < myInventory.entries(); i++)
	    {
		if (myInventory(i)->defn().isflag)
		    delete myInventory(i);
		else
		    myInventory(i)->move(pos());
	    }
	    myInventory.clear();
	}

	if (src && src->isAvatar())
	{
	    // Award our score.
	    if (!isAvatar())
	    {
		// Ignore suicide, give us the blood!
		src->gainHP(getMaxHP());
	    }

	}

	// Flash the screen
	pos().postEvent(EVENTTYPE_FOREBACK, ' ', ATTR_INVULNERABLE);

	// Note that avatar doesn't actually die...
	if (!isAvatar())
	{
	    // Anyone who witnessed this can yell.
	    if (src && src->isAvatar() && isFriends(src))
	    {
		MOBLIST		allmobs;

		pos().map()->getAllMobs(allmobs);

		for (int i = 0; i < allmobs.entries(); i++)
		{
		    if (allmobs(i)->pos().isFOV() && allmobs(i) != this &&
			!allmobs(i)->isAvatar() &&
			allmobs(i)->alive() &&
			allmobs(i)->isFriends(src) &&
			!allmobs(i)->hasItem(ITEM_BLIND))
		    {
			allmobs(i)->actionYell(YELL_MURDERER);
			allmobs(i)->giftItem(ITEM_ENRAGED);
			break;
		    }
		}
	    }

	    // Don't actually delete, but die.
	    MAP		*map = pos().map();
	    ITEM	*corpse = ITEM::create(ITEM_CORPSE);
	    corpse->setMobType(getDefinition());

	    corpse->move(pos());

	    myPos.removeMob(this);
	    map->addDeadMob(this);
	    clearAllPos();
	    loseTempItems();
	}
	else
	{
	    // Make sure we drop our blind attribute..
	    loseTempItems();

	    // End any meditation
	    myMeditatePos = POS();

	    // No matter what, the source sees it (we may be meditating)
	    if (src)
		src->mySawVictory = true;

	    if (pos().isFOV())
	    {
		MOBLIST		allmobs;

		pos().map()->getAllMobs(allmobs);

		for (int i = 0; i < allmobs.entries(); i++)
		{
		    if (allmobs(i)->pos().isFOV())
			allmobs(i)->mySawVictory = true;
		}
	    }
	}
	return true;
    }

    // They lived.  They get the chance to yell.
    if (src && src->isAvatar() && isFriends(src) && !isAvatar())
    {
	// This is a free action.
	actionYell(YELL_MURDERER);
	giftItem(ITEM_ENRAGED);
    }

    // Flash that they are hit.
    if (hits)
	pos().postEvent(EVENTTYPE_FOREBACK, ' ', ATTR_HEALTH);

    loseHP(hits);
    return false;
}

void
MOB::kill()
{
    applyDamage(0, getHP(), ELEMENT_PHYSICAL, ATTACKSTYLE_INTERNAL);
}

VERB_PERSON
MOB::getPerson() const
{
    if (isAvatar())
	return VERB_YOU;

    return VERB_IT;
}

bool
MOB::isFriends(const MOB *other) const
{
    if (other == this)
	return true;

    if (hasItem(ITEM_ENRAGED) || other->hasItem(ITEM_ENRAGED))
	return false;
    
    if (isAvatar())
    {
	if (other->defn().isfriendly)
	    return true;
	else
	    return false;
    }

    // Only the avatar is evil!
    if (defn().isfriendly)
    {
	return true;
    }
    else
    {
	// Monsters hate the avtar!
	if (other->isAvatar())
	    return false;
    }
    return true;
}

AI_NAMES
MOB::getAI() const
{
    return (AI_NAMES) defn().ai;
}

void
MOB::reportSquare(POS t)
{
    if (!isAvatar())
	return;

    if (t.mob() && t.mob() != this)
    {
	formatAndReport("%S <see> %O.", t.mob());
    }
    if (t.item())
    {
	ITEMLIST	itemlist;

	t.allItems(itemlist);
	if (itemlist.entries())
	{
	    BUF		msg;
	    msg.strcpy("%S <see> ");
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
	    formatAndReport(msg);
	}
    }

    if (t.defn().describe)
    {
	formatAndReport("%S <see> %O.", t.defn().legend);
    }
}

void
MOB::meditateMove(POS t)
{
    myMeditatePos = t;
    reportSquare(t);
}

bool
MOB::actionBump(int dx, int dy)
{
    MOB		*mob;
    POS		 t;

    // Stand in place.
    if (!dx && !dy)
	return true;

    if (isMeditating())
    {
	// We are free of our body!
	t = myMeditatePos.delta(dx, dy);
	if (t.defn().ispassable)
	{
	    meditateMove(t);
	    return true;
	}
	// Wall slide...
	if (dx && dy && isAvatar())
	{
	    // Try to wall slide, cause we are pretty real time here
	    // and it is frustrating to navigate curvy passages otherwise.
	    t = myMeditatePos.delta(dx, 0);
	    if (!rand_choice(2) && t.defn().ispassable)
	    { meditateMove(t); return true; }

	    t = myMeditatePos.delta(0, dy);
	    if (t.defn().ispassable)
	    { meditateMove(t); return true; }

	    t = myMeditatePos.delta(dx, 0);
	    if (t.defn().ispassable)
	    { meditateMove(t); return true; }
	}
	else if ((dx || dy) && isAvatar())
	{
	    // If we have
	    // ..
	    // @#
	    // ..
	    // Moving right we want to slide to a diagonal.
	    int		sdx, sdy;

	    // This bit of code is too clever for its own good!
	    sdx = !dx;
	    sdy = !dy;

	    t = myMeditatePos.delta(dx+sdx, dy+sdy);
	    if (!rand_choice(2) && t.defn().ispassable)
	    { meditateMove(t); return true; }

	    t = myMeditatePos.delta(dx-sdx, dy-sdy);
	    if (t.defn().ispassable)
	    { meditateMove(t); return true; }

	    t = myMeditatePos.delta(dx+sdx, dy+sdy);
	    if (t.defn().ispassable)
	    { meditateMove(t); return true; }
	}
	return false;
    }

    t = pos().delta(dx, dy);

    // If we are swallowed, we must attack.
    if (isSwallowed())
	return actionMelee(dx, dy);
    
    mob = t.mob();
    if (mob)
    {
	// Either kill or chat!
	if (mob->isFriends(this))
	{
	    if (isAvatar())
		return actionChat(dx, dy);
	    // Otherwise we just walk and bump.
	}
	else
	{
	    return actionMelee(dx, dy);
	}
    }

    // See if it is a shop, then shop.
    if (t.defn().shop != MOB_NONE)
    {
	glbEngine->shopRequest((MOB_NAMES) t.defn().shop);
	return true;
    }

    // See if it is a staircase, we must climb them!
    if (t.tile() == TILE_DOWNSTAIRS)
    {
	if (isAvatar())
	{
	    // Check if we have the macguffin!
	    if (!hasItem((ITEM_NAMES)glb_questdefs[glbCurrentQuest].questitem))
	    {
		formatAndReport("If %U return without %O, they will no doubt just lock you away in a vault again!",
			glb_itemdefs[glb_questdefs[glbCurrentQuest].questitem].name);

		postEvent( (EVENTTYPE_NAMES) (EVENTTYPE_SHOUT | EVENTTYPE_LONG),
				' ', ATTR_SHOUT,
				"Not yet!");
		return false;
	    }
	    else
	    {
		// Ready to win
		glbEngine->queue().append(COMMAND(ACTION_ENDQUEST, glbCurrentQuest));
		formatAndReport("You escape with the %O!",
			glb_itemdefs[glb_questdefs[glbCurrentQuest].questitem].name);
	    }
	}
    }

    // No mob, see if we can move that way.
    // We let actionWalk deal with unable to move notification.
    return actionWalk(dx, dy);
}

bool
MOB::actionMeditate()
{
    if (isMeditating())
    {
	formatAndReport("%S <stop> meditating.");
	myMeditatePos = POS();
	// we don't want the user to be able to use up a fast
	// turn by doing this!
	PHASE_NAMES		phase;

	phase = spd_getphase();
	if (phase == PHASE_FAST || phase == PHASE_QUICK)
	{
	    mySkipNextTurn = true;
	}
    }
    else
    {
	if (pos().tile() != TILE_MEDITATIONSPOT)
	{
	    formatAndReport("This location is not tranquil enough to support meditation.");
	    return false;
	}
	formatAndReport("%S <close> %r eyes and meditate.");
    }
    return true;
}

void
MOB::searchOffset(int dx, int dy, bool silent)
{
    POS		square = pos().delta(dx, dy);

    if (square.isTrap())
    {
	TRAP_NAMES		trap = (TRAP_NAMES) rand_choice(NUM_TRAPS);

	formatAndReport("%S find %O and disarm it.", glb_trapdefs[trap].name);
	square.clearTrap();

	square.postEvent((EVENTTYPE_NAMES)(EVENTTYPE_ALL | EVENTTYPE_LONG), 
			glb_trapdefs[trap].sym,
			(ATTR_NAMES) glb_trapdefs[trap].attr);
    }
    else if (square.tile() == TILE_SECRETDOOR)
    {
	formatAndReport("%S <reveal> a secret door.");
	square.setTile(TILE_DOOR);
	square.postEvent(EVENTTYPE_FOREBACK, '+', ATTR_SEARCH);
    }
    else
    {
	if (!silent)
	    square.postEvent(EVENTTYPE_FOREBACK, ' ', ATTR_SEARCH);
    }
}

bool
MOB::actionSearch(bool silent)
{
    mySearchPower = 1;
    if (!silent)
	formatAndReport("%S <search>.");

    int		ir, r;

    r = mySearchPower;
    // Note the < r to avoid double counting corners.
    for (ir = -r; ir < r; ir++)
    {
	searchOffset(ir, -r, silent);
	searchOffset(r, ir, silent);
	searchOffset(-ir, r, silent);
	searchOffset(-r, -ir, silent);
    }

    return true;
}

bool
MOB::actionDropButOne(ITEM *item)
{
    if (!item)
	return false;

    ITEM		*butone;

    butone = splitStack(item);
    assert(butone != item);

    if (butone == item)
    {
	addItem(butone);
	return false;
    }

    // Swap the two meanings
    butone->setStackCount(item->getStackCount());
    item->setStackCount(1);

    formatAndReport("%S <drop> %O.", butone);

    // And drop
    butone->move(pos());

    return true;
}

bool
MOB::actionDrop(ITEM *item)
{
    if (!item)
	return false;

    if (item->getDefinition() == ITEM_FREYBLADE)
    {
	postEvent( (EVENTTYPE_NAMES) (EVENTTYPE_SHOUT | EVENTTYPE_LONG),
			' ', ATTR_SHOUT,
			"Not my precious!");
	formatAndReport("%S <decide> against being dropped.", item);
	return false;
    }

    // Drop any stuff we have.
    int i;
    bool	fail = true;
    for (i = 0; i < myInventory.entries(); i++)
    {
	// Ignore special case items..
	// We don't want to drop "blindness" :>
	if (myInventory(i)->defn().isflag)
	    continue;

	if (myInventory(i) == item)
	{
	    formatAndReport("%S <drop> %O.", myInventory(i));
	    fail = false;
	    myInventory(i)->move(pos());
	    myInventory.set(i, 0);
	}
    }
    myInventory.collapse();

    updateEquippedItems();

    if (fail)
	formatAndReport("%S <drop> nothing.");

    return true;
}

bool
MOB::actionQuaff(ITEM *item)
{
    if (!item)
	return false;
    if (!item->isPotion())
    {
	formatAndReport("%S cannot drink %O.", item);
	return false;
    }

    item = splitStack(item);

    postEvent( (EVENTTYPE_NAMES) (EVENTTYPE_SHOUT | EVENTTYPE_LONG),
		    ' ', ATTR_EMOTE,
		    "*quaff*");
    formatAndReport("%S <quaff> %O.", item);
    bool		interesting = false;
    switch (item->getMagicClass())
    {
	case POTION_HEAL:
	    if (!isFullHealth())
	    {
		interesting = true;
		formatAndReport("%S <look> healthier.");
		gainHP(15);
	    }
	    break;

	case POTION_SPEED:
	    if (!hasItem(ITEM_QUICKBOOST))
	    {
		interesting = true;
		giftItem(ITEM_QUICKBOOST);
	    }
	    break;

	case POTION_CURE:
	    if (hasItem(ITEM_POISON))
	    {
		ITEM	*p = lookupItem(ITEM_POISON);
		removeItem(p);
		interesting = true;
	    }
	    break;

	case POTION_POISON:
	    if (!hasItem(ITEM_POISON))
		interesting = true;
	    giftItem(ITEM_POISON);
	    {
		ITEM	*p = lookupItem(ITEM_POISON);
		if (p)
		    p->addTimer(10);
	    }
	    break;

	case POTION_JUICE:
	    // This is mean :>
	    break;

	case POTION_ACID:
	    interesting = true;
	    formatAndReport("%S <be> burned by the acid!");
	    applyDamage(this, 10, ELEMENT_ACID, ATTACKSTYLE_INTERNAL);
	    break;
    }

    if (interesting)
    {
	item->markMagicClassKnown();
    }
    else
    {
	formatAndReport("Nothing happens.");
    }

    delete item;

    return true;
}

bool
MOB::actionThrow(ITEM *item, int dx, int dy)
{
    if (!item)
	return false;
    if (!item->isPotion())
    {
	msg_format("%S <be> not sufficiently aerodynamic.", item);
	return false;
    }

    item = splitStack(item);

    formatAndReport("%S <throw> %O.", item);
    bool		interesting = false;

    POTION_OP		op(this, (POTION_NAMES)item->getMagicClass(), &interesting);

    u8			sym;
    ATTR_NAMES		attr;
    item->getLook(sym, attr);

    if (!doRangedAttack(5, 2, dx ,dy, '~', attr, "hit", op))
    {
	addItem(item);
	return false;
    }

    if (interesting)
    {
	item->markMagicClassKnown();
    }

    delete item;

    return true;
}

bool
MOB::actionEat(ITEM *item)
{
    if (!item)
	return false;

    if (!item->isFood())
    {
	formatAndReport("%S cannot eat %O.", item);
	return false;
    }

    item = splitStack(item);

    formatAndReport("%S <eat> %O.", item);
    // Very minimalistic effect, but makes the food items
    // appear worthwhile.
    gainHP(1);

    delete item;

    return true;
}

bool
MOB::actionBreak(ITEM *item)
{
    if (!item)
	return false;

    if (!item->isRanged())
    {
	formatAndReport("%S cannot break %O.", item);
	return false;
    }

    formatAndReport("%S <break> %O.", item);

    postEvent( (EVENTTYPE_NAMES) (EVENTTYPE_SHOUT | EVENTTYPE_LONG),
		    ' ', ATTR_EMOTE,
		    "*snap*");

    item->setBroken(true);

    return true;
}

bool
MOB::actionWear(ITEM *item)
{
    if (!item)
	return false;

    if (item->isRanged())
    {
	formatAndReport("You already know the best bow to use.");
	return false;
    }

    if (item->isMelee())
    {
	formatAndReport("You already know the best weapon to use.");
	return false;
    }

    if (item->isArmour())
    {
	formatAndReport("You already know the best armour to wear.");
	return false;
    }

    if (!item->isRing())
    {
	formatAndReport("%S cannot wear %O.", item);
	return false;
    }

    ITEM		*oldring = lookupRing();

    if (oldring)
    {
	formatAndReport("%S <take> off %O.", oldring);
	oldring->setEquipped(false);
    }

    // If they are the same, the user wanted to toggle!
    if (oldring != item)
    {
	formatAndReport("%S <put> on %O.", item);
	item->setEquipped(true);
    }

    return true;
}

const char *
getYellMessage(YELL_NAMES yell)
{
    const char *yellmsg;

    switch (yell)
    {
	case YELL_KEEPOUT:
	{
	    const char *msg[] =
	    {		// KEEPOUT
		"Keep out!",
		"Away!",
		"Leave!",
		"No Farther!",
		0
	    };
	    yellmsg = rand_string(msg);
	    break;
	}
	case YELL_MURDERER:
	{
	    const char *msg[] =
	    {		// MURDERER
		"Killer!",
		"Murderer!",
		0
	    };
	    yellmsg = rand_string(msg);
	    break;
	}
	case YELL_INVADER:
	{
	    const char *msg[] =
	    {		// INVADER
		"Invader!",
		"ALARM!",
		"ALARM!",
		"Code Yellow!",	// For yummy gold.
		0
	    };
	    yellmsg = rand_string(msg);
	    break;
	}
	case YELL_RANGED:
	{
	    const char *msg[] =
	    {		// RANGED
		"Far Threat!",
		"Code Bow!",	// Kobolds not that imaginative after all
		"Incoming!",
		"Archers!",	
		0
	    };
	    yellmsg = rand_string(msg);
	    break;
	}
	case YELL_KILL:
	{
	    const char *msg[] =
	    {		// KILL
		"Kill it now!",
		"Code Gold!",
		"Kill!",
		"Eviscerate It!",
		"Kill!",
		"Kill!",
		0
	    };
	    yellmsg = rand_string(msg);
	    break;
	}
	case YELL_KILLCHASE:
	{
	    const char *msg[] =
	    {		// KILL
		"No Mercy!",
		"Code Blood!",
		"Hunt It Down!",
		0
	    };
	    yellmsg = rand_string(msg);
	    break;
	}
	case YELL_LOCATION:
	{
	    const char *msg[] =
	    {		// LOCATION
		"It's here!",
		"Foe Sighted!",
		"Over here!",
		"I see it!",
		0
	    };
	    yellmsg = rand_string(msg);
	    break;
	}
	case YELL_HEARDLOCATION:
	{
	    const char *msg[] =
	    {		// LOCATION_HEARD 
		"It's there!",
		"Foe Located!",
		"Over there!",
		0
	    };
	    yellmsg = rand_string(msg);
	    break;
	}
	case YELL_TAUNT:
	{
	    const char *msg[] =
	    {		// LOCATION_HEARD 
		"Coward!",
		"You Flea!",
		"Mendicant!",
		"Wimp!",
		"Fool!",
		"Urchin!",
		0
	    };
	    yellmsg = rand_string(msg);
	    break;
	}
	case YELL_VICTORY:
	{
	    const char *msg[] =
	    {		// LOCATION_HEARD 
		"Victory!",
		"The @ is dead!",
		"Huzzah!",
		"Hooray!",
		"Gold!",
		0
	    };
	    yellmsg = rand_string(msg);
	    break;
	}
    }
    return yellmsg;
}

bool
MOB::actionYell(YELL_NAMES yell)
{
    const char	*yellflavour[] =
    {
	"%S <yell>: \"%o\"",
	"%S <shout>: \"%o\"",
	"%S <curse>: \"%o\"",
	0
    };
    const char *yellmsg = getYellMessage(yell);

    MOBLIST	hearmobs;

    pos().getAllConnectedRoomMobs(hearmobs);

    for (int i = 0; i < hearmobs.entries(); i++)
    {
	if (hearmobs(i)->isAvatar())
	{
	    // Don't use line of sight, but the same logic we use.
	    msg_format(rand_string(yellflavour), this, yellmsg);
	    postEvent( (EVENTTYPE_NAMES) (EVENTTYPE_SHOUT | EVENTTYPE_LONG),
			    ' ', ATTR_SHOUT,
			    yellmsg);
	}
	if (hearmobs(i) == this)
	    continue;
	hearmobs(i)->myHeardYell[yell] = true;
	if (hearmobs(i)->pos().roomId() == pos().roomId())
	{
	    // Extra flag to avoid spurious re-shouts
	    hearmobs(i)->myHeardYellSameRoom[yell] = true;
	}
    }

    return true;
}

bool
MOB::actionRotate(int angle)
{
    myPos = myPos.rotate(angle);
    if (isMeditating())
	myMeditatePos = myMeditatePos.rotate(angle);
    // This is always a free action.
    return false;
}

bool
MOB::actionClimb()
{
    TILE_NAMES		tile;

    tile = pos().tile();

    if (tile == TILE_DOWNSTAIRS)
    {
	// Climbing is interesting.
	myFleeCount = 0;
	clearBoredom();
 
	msg_format("%S <climb> down... to nowhere!", this);
	return true;
    }
    else if (tile == TILE_UPSTAIRS)
    {
	msg_format("%S <climb> up... to nowhere!", this);
	return true;
    }
    else if (tile == TILE_FOREST)
    {
	msg_format("%S <climb> a tree... and <fall> down!", this);
	return true;
    }

    msg_format("%S <see> nothing to climb here.", this);

    return false;
}

bool
MOB::actionChat(int dx, int dy)
{
    MOB		*victim;

    // This should never occur, but to avoid
    // embarassaments...
    if (!isAvatar())
	return false;
    
    victim = pos().delta(dx, dy).mob();
    if (!victim)
    {
	// Talk to self
	formatAndReport("%S <talk> to %O!", this);
	return false;
    }

    formatAndReport("%S <chat> with %O.", victim);
    if (victim->getDefinition() == MOB_SERVANT)
    {
	victim->postEvent( (EVENTTYPE_NAMES) (EVENTTYPE_SHOUT | EVENTTYPE_LONG),
		    ' ', ATTR_SHOUT,
		    "Must clean, not chat!");
    }
    else if (victim->getDefinition() == MOB_GUARD)
    {
	victim->postEvent( (EVENTTYPE_NAMES) (EVENTTYPE_SHOUT | EVENTTYPE_LONG),
		    ' ', ATTR_SHOUT,
		    "Sorry, I'm on duty.");
    }
    else
    {
	// Go shopping!
	glbEngine->shopRequest(victim->getDefinition());
    }
    
    return true;
}

bool
MOB::actionMelee(int dx, int dy)
{
    MOB		*victim;

    // If we are swallowed, attack the person on our square.
    if (isSwallowed())
	dx = dy = 0;

    POS		 t = pos().delta(dx, dy);

    victim = t.mob();
    if (!victim)
    {
	// Swing in air!
	msg_format("%S <swing> at empty air!", this);
	return false;
    }

    if (!victim->alive())
    {
	// The end game avatar, just silently ignore
	assert(!isAvatar());
	return false;
    }

    // Shooting is interesting.
    clearBoredom();
 
    int		damage;
    bool	victimdead;

    damage = getMeleeDamage();

    if (isAvatar())
    {
	// We have to pay for the attack in blood...
	// However, in order to avoid rather emabarrassing problems,
	// we let you still attack if you have only one blood left.
	if (getHP() > 1)
	    loseHP(1);
    }


    // attempt to kill victim
    if (damage || (defn().melee_item != ITEM_NONE))
    {
	msg_format("%S %v %O.", this, getMeleeVerb(),
				victim);
    }
    else
    {
	msg_format("%S <miss> %O.", this, victim);
    }

    victimdead = victim->applyDamage(this, damage,
				    getMeleeElement(),
				    ATTACKSTYLE_MELEE);
    
    // Vampires regenerate!
    if (defn().isvampire)
    {
	// Avoids max!!
	myHP += damage;
    }

    // Grant our item...
    if (!victimdead)
    {
	ITEM_NAMES	item;

	item = (ITEM_NAMES) defn().melee_item;
	if (item != ITEM_NONE)
	{
	    ITEM	*i;

	    // Exclusive items you should only get one of so they
	    // don't stack.
	    if (!glb_itemdefs[item].exclusive ||
		!victim->hasItem(item))
	    {
		i = ITEM::create(item);
		if (i)
		    victim->addItem(i);
	    }
	}

	// Swallow the victim
	// Seems silly to do this on a miss.
	if (defn().swallows && damage)
	{
	    victim->setSwallowed(true);
	    victim->move(pos(), true);
	}

	// Steal something
	if (defn().isthief)
	{
	    ITEM		*item;
	    
	    // Get a random item from the victim
	    item = victim->getRandomItem();
	    if (item) 
	    {
		msg_format("%S <steal> %O.", this, item);
		// Theft successful.
		myFleeCount += 30;
		victim->removeItem(item, true);
		addItem(item);
	    }
	}
    }
	
    return true;
}

const char *
MOB::getMeleeVerb() const
{
    ITEM	*w;

    w = lookupWeapon();
    if (!w)
	return defn().melee_verb;

    return w->defn().melee_verb;
}

ELEMENT_NAMES
MOB::getMeleeElement() const
{
    if (lookupWeapon())
	return ELEMENT_PHYSICAL;

    return (ELEMENT_NAMES) defn().melee_element;
}

const char *
MOB::getMeleeWeaponName() const
{
    ITEM	*w;

    w = lookupWeapon();
    if (!w)
	return defn().melee_name;

    return w->defn().name;
}

int
MOB::getMeleeDamage() const
{
    DPDF	damage;

    damage = getMeleeDPDF();

    return damage.evaluate();
}

DPDF
MOB::getMeleeDPDF() const
{
    DPDF	dpdf(0);

    ITEM	*weapon;

    weapon = lookupWeapon();
    if (weapon)
    {
	return weapon->getMeleeDPDF();
    }

    dpdf = defn().melee_damage.buildDPDF();

    dpdf *= (defn().melee_chance) / 100.0;

    return dpdf;
}

int
MOB::getRangedDamage() const
{
    DPDF	damage;

    damage = getRangedDPDF();

    return damage.evaluate();
}

DPDF
MOB::getRangedDPDF() const
{
    ITEM	*weapon;

    weapon = lookupWand();
    if (weapon)
    {
	return weapon->getRangeDPDF();
    }

    DPDF	dpdf(0);

    dpdf = defn().range_damage.buildDPDF();

    double		tohit;

    tohit = defn().range_chance / 100.0;
    dpdf *= tohit;

    return dpdf;
}

int
MOB::getRangedRange() const
{
    int		range;

    ITEM	*weapon;

    weapon = lookupWand();
    if (weapon)
    {
	return weapon->getRangeRange();
    }

    range = defn().range_range;

    return range;
}

const char *
MOB::getRangedWeaponName() const
{
    ITEM	*w;

    w = lookupWand();
    if (!w)
	return defn().range_name;

    return w->defn().name;
}

ELEMENT_NAMES
MOB::getRangedElement() const
{
    ITEM	*w;

    w = lookupWand();
    if (!w)
	return (ELEMENT_NAMES) defn().range_element;

    return ELEMENT_PHYSICAL;
}

void
MOB::getRangedLook(u8 &symbol, ATTR_NAMES &attr) const
{
    symbol = defn().range_symbol;
    attr = (ATTR_NAMES) defn().range_attr;

    // Check out item, if any.
    ITEM		*i;

    i = lookupWand();

    if (i)
    {
	symbol = i->defn().range_symbol;
	attr = (ATTR_NAMES)i->defn().range_attr;
    }
}


template <typename OP>
bool
MOB::doRangedAttack(int range, int area, int dx, int dy, 
		u8 symbol, ATTR_NAMES attr,
		const char *verb, OP op)
{
    int		rangeleft;
    MOB		*victim;

    // Check for friendly kill.
    victim = pos().traceBullet(range, dx, dy, &rangeleft);

    if (victim && victim->isFriends(this))
    {
	// Not a clear shot!
	if (isAvatar())
	{
	    if (victim == this)
		msg_report(text_lookup("fire", getRawName()));
	    else
		msg_report("Have care where you aim that!");
	    // We have special messages.
	    return false;
	}

	// Avoid friendly fire
	return false;
    }

    // Shooting is interesting.
    clearBoredom();
 
    pos().displayBullet(range,
			dx, dy,
			symbol, attr,
			true);

    if (!victim)
    {
	// Shoot at air!
	// But, really, the fireball should explode!
	POS		vpos;
	vpos = pos().traceBulletPos(range, dx, dy, true);

	// Apply damage to everyone in range.
	vpos.fireball(this, area, symbol, attr, op); 

	return true;
    }

    // Attemp to kill victim
    msg_format("%S %v %O.", this, verb, victim);

    // NOTE: Bad idea to call a function on victim->pos() as
    // that likely will be deleted :>
    POS		vpos = victim->pos();

    // Apply damage to everyone in range.
    vpos.fireball(this, area, symbol, attr, op); 
    
    // The following code will keep the flame going past the target
#if 0
    while (isAvatar() && rangeleft && area == 1)
    {
	// Ensure our dx/dy is copacetic.
	vpos.setAngle(pos().angle());
	victim = vpos.traceBullet(rangeleft, dx, dy, &rangeleft);

	if (victim)
	{
	    msg_format("%S %v %O.", this, defn().range_verb,
				    victim);
	    vpos = victim->pos();
	    vpos.fireball(this, area, getRangedDPDF(), symbol, attr); 
	}
	else
	    rangeleft = 0;
    }
#endif

    return true;
}

bool
MOB::actionFire(int dx, int dy)
{
    // Check for no ranged weapon.
    if (!defn().range_valid && !lookupWand())
    {
	msg_format("%S <lack> a ranged attack!", this);
	return false;
    }

    ITEM_NAMES		ammo = ITEM_NONE;

    if (isAvatar() && lookupWand())
	ammo = (ITEM_NAMES) lookupWand()->defn().ammo;

    if (ammo != ITEM_NONE && !hasItem(ammo))
    {
	BUF		msg, ammoname;

	ammoname = gram_makeplural(glb_itemdefs[ammo].name);
	msg.sprintf("%%S <be> out of %s!", ammoname.buffer());
	formatAndReport(msg);
	return false;
    }

    // No suicide.
    if (!dx && !dy)
    {
	msg_format("%S <decide> not to aim at %O.", this, this);
	return false;
    }
    
    // If swallowed, rather useless.
    if (isSwallowed())
    {
	msg_format("%S <do> not have enough room inside here.", this);
	return false;
    }

    u8		symbol;
    ATTR_NAMES	attr;
    getRangedLook(symbol, attr);

    // Use up an arrow!
    if (ammo != ITEM_NONE)
    {
	ITEM	*arrow;
	arrow = lookupItem(ammo);
	if (arrow)
	{
	    arrow->decStackCount();
	    if (!arrow->getStackCount())
	    {
		BUF		ammoname;

		ammoname = gram_makeplural(glb_itemdefs[ammo].name);

		removeItem(arrow, true);
		formatAndReport("%S <shoot> the last of %r %o.", ammoname);
		delete arrow;
	    }
	}
    }

    ATTACK_OP	op(this, getRangedDPDF(), getRangedElement());

    return doRangedAttack(getRangedRange(), 1, dx, dy, 
		    symbol, attr,
		    defn().range_verb, op);
}

void
MOB::triggerManaUse(SPELL_NAMES spell, int manacost)
{
}

bool
MOB::actionShop(SHOP_NAMES shop, int goldcost, int parm)
{
    ITEM		*gold;
    int			 ngold;

    gold = lookupItem(ITEM_GOLD);
    
    ngold = 0;
    if (gold)
	ngold = gold->getStackCount();

    if (ngold < goldcost)
    {
	BUF		buf;
	buf.sprintf("%%S <lack> sufficient gold, having only %d coins.",
		    ngold);
	formatAndReport(buf);
	return true;
    }

    if (goldcost)
    {
	gold->setStackCount(gold->getStackCount() - goldcost);
    }

    switch (shop)
    {
	case SHOP_HEAL:
	    if (parm)
		formatAndReport("%S <rest> long and deeply.  %r wounds are tended and heal quickly.");
	    else
		formatAndReport("%S <take> a short break from the pressures of adventure.");
	    gainHP(parm);
	    break;

	case SHOP_FIX:
	{
	    ITEM		*item = getItemFromNo(parm);
	    if (!item)
		formatAndReport("Thin air fails to be repaired!");
	    else
	    {
		formatAndReport("%r %o is repaired.", item);
		item->setBroken(false);
		updateEquippedItems();
	    }
	    break;
	}

	case SHOP_ID:
	{
	    ITEM		*item = getItemFromNo(parm);
	    if (!item)
		formatAndReport("That is air, it is!");
	    else
	    {
		if (item->getStackCount() > 1)
		    formatAndReport("%r %o are identified.", item);
		else
		    formatAndReport("%r %o is identified.", item);
		item->markMagicClassKnown();
		if (item->getStackCount() > 1)
		    formatAndReport("They are %o!", item);
		else
		    formatAndReport("It is %o!", item);
		glbEngine->popupText(item->getLongDescription());
	    }
	    break;
	}

	case SHOP_BUY:
	{
	    ITEM		*item = ITEM::create((ITEM_NAMES) parm);
	    formatAndReport("%S <receive> %O.", item);
	    addItem(item);
	    break;
	}

	case SHOP_REMOVEITEM:
	{
	    ITEM		*item = lookupItem((ITEM_NAMES) parm);
	    // Removing flag effects is noisy.
	    removeItem(item);
	    delete item;
	    break;
	}

	case SHOP_GOSSIP:
	{
	    formatAndReport("%S patiently <listen> to a long spiel.");
	    glbEngine->popupText(text_lookup("gossip", glb_gossipdefs[parm].name));
	    break;
	}

	case SHOP_MAP:
	{
	    formatAndReport("%S <step> back from the map.");
	    break;
	}

	case SHOP_NONE:
	case NUM_SHOPS:
	    assert(!"Invalid shop");
	    break;
    }

    return true;
}

bool
MOB::actionMagicMove(MAGICMOVE_NAMES move, int dx, int dy)
{
    // Pay the piper...
    int			hpcost = 0;

    if (!dx && !dy)
    {
	formatAndReport("Without momentum, this move is useless!");
	return false;
    }

    hpcost = glb_magicmovedefs[move].blood;

    if (hpcost >= getHP())
    {
	postEvent( (EVENTTYPE_NAMES) (EVENTTYPE_SHOUT | EVENTTYPE_LONG),
			' ', ATTR_SHOUT,
			"Please, No More!");
	formatAndReport("Without more blood, %O would kill your host.",
			    glb_magicmovedefs[move].name);

	return false;
    }

    // Apply the damage...
    const char *pattern;
    POS		attackstart = pos();

    pattern = glb_magicmovedefs[move].pattern;
    for (int y = 0; y < 5; y++)
    {
	for (int x = 0; x < 7; x++)
	{
	    int		mdx, mdy;

	    mdx = x - 3;
	    mdy = y - 2;

	    // Rotate by our dx/dy.
	    // mdx, mdy are in the basis (1, 0), (0, 1)
	    // We want our new basis to be (dx, dy), (-dy, dx)
	    int 	ldx, ldy;

	    ldx = dx * mdx - dy * mdy;
	    ldy = dy * mdx + dx * mdy;
	    
	    POS		target = attackstart.delta(ldx, ldy);

	    if (*pattern == '@')
	    {
		// Test to see if we can move.
		if ((target.mob() != this) && !canMove(target))
		{
		    postEvent( (EVENTTYPE_NAMES) (EVENTTYPE_SHOUT | EVENTTYPE_LONG),
				' ', ATTR_SHOUT,
				"No room!");
		    formatAndReport("No room to complete this move.");

		    return false;
		}
	    }
	    pattern++;
	}
    }

    formatAndReport("%S <perform> %O.",
			glb_magicmovedefs[move].name);

    // Do not apply damage, this avoids invulnerability!!
    // We also don't want the hit spam.
    // applyDamage(this, hpcost, ELEMENT_NONE, ATTACKSTYLE_INTERNAL);
    loseHP(hpcost);

    pattern = glb_magicmovedefs[move].pattern;
    for (int y = 0; y < 5; y++)
    {
	for (int x = 0; x < 7; x++)
	{
	    int		mdx, mdy;

	    mdx = x - 3;
	    mdy = y - 2;

	    // Rotate by our dx/dy.
	    // mdx, mdy are in the basis (1, 0), (0, 1)
	    // We want our new basis to be (dx, dy), (-dy, dx)
	    int 	ldx, ldy;

	    ldx = dx * mdx - dy * mdy;
	    ldy = dy * mdx + dx * mdy;
	    
	    POS		target = attackstart.delta(ldx, ldy);

	    if (*pattern == '*')
	    {
		MOB		*victim = target.mob();
		if (victim && victim->alive())
		{
		    msg_format("%S %v %O.", this, getMeleeVerb(),
					    victim);
		    victim->applyDamage(this,
					glb_magicmovedefs[move].damage,
					getMeleeElement(),
					ATTACKSTYLE_MELEE);
		}
		else
		{
		    target.postEvent(EVENTTYPE_FORESYM, '*', ATTR_RED);
		}
	    }
	    else if (*pattern == '!')
	    {
		MOB		*victim = target.mob();
		if (victim && victim->alive() && !victim->hasItem(ITEM_BLIND))
		{
		    msg_format("%S %v %O.", this, "blind",
					    victim);
		    victim->giftItem(ITEM_BLIND);
		}
		target.postEvent(EVENTTYPE_FORESYM, '*', ATTR_YELLOW);
	    }
	    else if (*pattern == '@')
	    {
		// Move here!
		if (canMove(target))
		{
		    this->move(target);
		}
	    }
	    pattern++;
	}
    }
    return true;
}

bool
MOB::actionCast(SPELL_NAMES spell, int dx, int dy)
{
    // Pay the piper...
    int			manacost = 0, hpcost = 0;

    manacost = glb_spelldefs[spell].mana;
    
    // Now pay with health...
    if (manacost > getMP())
    {
	hpcost = manacost - getMP();
	manacost = getMP();
	if (hpcost >= getHP())
	{
	    // Don't let people suicide.
	    formatAndReport("Without more mana, casting %O would kill %S!",
			    glb_spelldefs[spell].name);
	    msg_newturn();
	    return false;
	}
    }

    if (hpcost)
    {
	formatAndReport("Paying with %r own life, %S <cast> %O.",
			    glb_spelldefs[spell].name);
    }
    else
    {
	// Less dramatic.
	formatAndReport("%S <cast> %O.",
			    glb_spelldefs[spell].name);
    }

    applyDamage(this, hpcost, ELEMENT_NONE, ATTACKSTYLE_INTERNAL);
    gainMP(-manacost);
    
    triggerManaUse(spell, manacost);

    return true;
}

bool
MOB::actionWalk(int dx, int dy)
{
    MOB		*victim;
    POS		 t;

    // If we are swallowed, we can't move.
    if (isSwallowed())
	return false;
    
    t = pos().delta(dx, dy);
    
    victim = t.mob();
    if (victim)
    {
	// If we are the avatar, screw being nice!
	if (isAvatar() && !isFriends(victim))
	    return actionMelee(dx, dy);
	// If it is thet avatar we are bumping into, screw being
	// nice.
	if (victim->isAvatar() && !isFriends(victim))
	    return actionMelee(dx, dy);

	// formatAndReport("%S <bump> into %O.", victim);
	// Bump into the mob.

	// Nah, tell the map we want a delayed move.
	return pos().map()->requestDelayedMove(this, dx, dy);
    }

    // Check to see if we can move that way.
    if (canMove(t))
    {
	TILE_NAMES	tile;
	
	// Determine if it is a special move..
	tile = t.tile();
	if (t.defn().isdiggable &&
	    !t.defn().ispassable &&	
	    defn().candig)
	{
	    return t.digSquare();
	}

	POS		opos = pos();
	
	// Move!
	move(t);

	// If we are a breeder, chance to reproduce!
	if (defn().breeder)
	{
	    static  int		lastbreedtime = -10;

	    // Only breed every 7 phases at most so one can
	    // always kill off the breeders.
	    // Only breed while the player is in the dungeon
	    bool		 canbreed = false;

	    if (getAvatar() && getAvatar()->alive())
	    {
		canbreed = true;
	    }

	    // Hard cap
	    int		mobcount = pos().map()->getMobCount(getDefinition());
	    if (mobcount > 50)
		canbreed = false;

	    // Every new breeder increases the chance of breeding
	    // by 1 roll.  Thus we need at least the base * mobcount
	    // to just keep the breeding chance linear.
	    if (canbreed && 
		(lastbreedtime < spd_gettime() - 12) &&
		!rand_choice(3 + 3*mobcount))
	    {
		MOB		*child;
		
		formatAndReport("%S <divide>!");
		child = MOB::create(getDefinition());
		child->move(opos);
		lastbreedtime = spd_gettime();
	    }
	}
	
	// If we are the avatar and something is here, pick it up.
	if (isAvatar() && t.item())
	    actionPickup();

	// Avatars set off traps because they are clumsy
	bool		hadtrap = false;
	if (isAvatar() && t.isTrap())
	{
	    hadtrap = true;
	    t.clearTrap();
	    TRAP_NAMES		trap = (TRAP_NAMES) rand_choice(NUM_TRAPS);

	    t.postEvent((EVENTTYPE_NAMES)(EVENTTYPE_ALL | EVENTTYPE_LONG), 
			    glb_trapdefs[trap].sym,
			    (ATTR_NAMES) glb_trapdefs[trap].attr);
	    DPDF	dpdf(0);

	    formatAndReport("%S <set> off %O!", glb_trapdefs[trap].name);

	    if (glb_trapdefs[trap].item != ITEM_NONE)
	    {
		ITEM		*item = ITEM::create((ITEM_NAMES)glb_trapdefs[trap].item);
		item->setStackCount(1);
		addItem(item);
	    }

	    dpdf = glb_trapdefs[trap].damage.buildDPDF();

	    if (!applyDamage(0, dpdf.evaluate(), 
			    (ELEMENT_NAMES) glb_trapdefs[trap].element,
			    ATTACKSTYLE_MELEE))
	    {
		postEvent( (EVENTTYPE_NAMES) (EVENTTYPE_SHOUT | EVENTTYPE_LONG),
				' ', ATTR_SHOUT,
				"Bah, a trap!");
	    }
	}

	// Apply searching
	if (!hadtrap && alive() && isAvatar())
	{
	    if (lookupRingName() == RING_SEARCH)
	    {
		actionSearch(true);
	    }
	}

	// Check for room switches.
	if (!hadtrap && alive() && isAvatar())
	{
	    ROOMTYPE_NAMES	roomtype;
	    roomtype = pos().roomType();
	    if (!glbRoomDiscovered[roomtype])
	    {
		glbRoomDiscovered[roomtype] = true;
	    }
	    else if (!glbRoomLooted[roomtype] &&
		     rand_chance(glb_roomtypedefs[roomtype].item_findchance) &&
		     !hasItem((ITEM_NAMES) glb_roomtypedefs[roomtype].item_find))
	    {
		glbRoomLooted[roomtype] = true;
		ITEM *item = ITEM::create((ITEM_NAMES)glb_roomtypedefs[roomtype].item_find);
		// Need to pick up before add item as we may need to
		// drop it right away!
		formatAndReport("%S <pick> up %O.", item);
		addItem(item);
		glbEngine->popupText(text_lookup("room", "loot"));
		glbEngine->popupText(item->getLongDescription());
	    }
	}
	
	return true;
    }
    else
    {
	if (dx && dy && isAvatar())
	{
	    // Try to wall slide, cause we are pretty real time here
	    // and it is frustrating to navigate curvy passages otherwise.
	    if (!rand_choice(2) && canMoveDir(dx, 0, false))
		if (actionWalk(dx, 0))
		    return true;
	    if (canMoveDir(0, dy, false))
		if (actionWalk(0, dy))
		    return true;
	    if (canMoveDir(dx, 0, false))
		if (actionWalk(dx, 0))
		    return true;
	}
	else if ((dx || dy) && isAvatar())
	{
	    // If we have
	    // ..
	    // @#
	    // ..
	    // Moving right we want to slide to a diagonal.
	    int		sdx, sdy;

	    // This bit of code is too clever for its own good!
	    sdx = !dx;
	    sdy = !dy;

	    if (!rand_choice(2) && canMoveDir(dx+sdx, dy+sdy, false))
		if (actionWalk(dx+sdx, dy+sdy))
		    return true;
	    if (canMoveDir(dx-sdx, dy-sdy, false))
		if (actionWalk(dx-sdx, dy-sdy))
		    return true;
	    if (canMoveDir(dx+sdx, dy+sdy, false))
		if (actionWalk(dx+sdx, dy+sdy))
		    return true;
	    
	}

	formatAndReport("%S <be> blocked by %O.", t.defn().legend);
	if (isAvatar())
	    msg_newturn();
	// Bump into a wall.
	return false;
    }
}

bool
MOB::actionPickup()
{
    ITEMLIST		items;
    ITEM		*bestitem = 0;

    pos().allItems(items);

    for (int i = 0; i < items.entries(); i++)
    {
	ITEM		*item = items(i);
	// Ignore corpses
	if (item->getDefinition() == ITEM_CORPSE)
	    continue;

	if (!bestitem)
	    bestitem = item;

	// Check if interesting.
	if (item->getDefinition() == glb_questdefs[glbCurrentQuest].questitem)
	    bestitem = item;
    }
    if (bestitem)
	return actionPickup(bestitem);
    return false;
}

bool
MOB::actionPickup(ITEM *item)
{
    if (!item)
    {
	formatAndReport("%S <grope> the ground foolishly.");
	return false;
    }

    if (item->pos() != pos())
    {
	formatAndReport("%S <be> too far away to pick up %O.", item);
	return false;
    }

    // You have no interest in mundane items.
    if (isAvatar())
    {
	if (item->getDefinition() == glb_questdefs[glbCurrentQuest].questitem)
	{
	    // Okay, may'be I'm interested!
	    postEvent( (EVENTTYPE_NAMES) (EVENTTYPE_SHOUT | EVENTTYPE_LONG),
			    ' ', ATTR_SHOUT,
			    "This is what I sought!");

	}
	else
	{
	    formatAndReport("%S <have> no interest in something as mundane as %O.", item);
	    return false;
	}
    }

    // Pick up the item!
    formatAndReport("%S <pick> up %O.", item);

    // No longer coveted!
    item->setInterestedUID(INVALID_UID);
    item->move(POS());

    if (item->defn().discardextra && hasItem(item->getDefinition()))
    {
	// Replace broken items...
	ITEM		*olditem = lookupItem(item->getDefinition());
	
	if (olditem->isBroken() && !item->isBroken())
	{
	    formatAndReport("%S <replaces> %O.", olditem);
	    olditem->setBroken(false);
	}

	formatAndReport("%S already <have> one of these, so %S <discard> it.");

	delete item;
    }
    else
	addItem(item);

    return true;
}

void
MOB::updateEquippedItems()
{
    ITEM		*item;

    if (!isAvatar())
	return;

    // Mark everything unequipped
    for (int i = 0; i < myInventory.entries(); i++)
    {
	item = myInventory(i);

	if (!item->isRing())
	    item->setEquipped(false);
    }

    // Equip our weapon and armour
    ITEM		*weap, *arm, *wand;
    weap = lookupWeapon();
    if (weap)
	weap->setEquipped(true);
    wand = lookupWand();
    if (wand)
	wand->setEquipped(true);
    arm = lookupArmour();
    if (arm)
	arm->setEquipped(true);

    // Discard everything that is inferior.
    ITEMLIST		discards;
    for (int i = 0; i < myInventory.entries(); i++)
    {
	item = myInventory(i);
	bool		oldbroken = item->isBroken();
	item->setBroken(false);

	if (arm != item && arm && item->isArmour() && aiLikeMoreArmour(arm, item) == arm)
	{
	    discards.append(item);
	}
	else if (weap != item && weap && item->isMelee() && aiLikeMoreWeapon(weap, item) == weap)
	{
	    discards.append(item);
	}
	else if (wand != item && wand && item->isRanged() && aiLikeMoreWand(wand, item) == wand)
	{
	    discards.append(item);
	}

	item->setBroken(oldbroken);
    }

    // Discard everything that is inferior.
    for (int i = 0; i < discards.entries(); i++)
    {
	formatAndReport("%S <discard> %O as it is now obsolete.", discards(i));
	// Can't call removeItem as it invokes us.
	myInventory.removePtr(discards(i));
	delete discards(i);
    }

    // Count total items
    int		total = 0;
    for (int i = 0; i < myInventory.entries(); i++)
    {
	if (myInventory(i)->defn().isflag)
	    continue;
	total++;
    }
    if (total > 15)
    {
	// Yay!  Chacne to use count down operator.
	// We want to run in reverse here.
	formatAndReport("%r backpack is out of room!");
	for (int i = myInventory.entries(); i --> 0; )
	{
	    item = myInventory(i);
	    if (item->defn().isflag)
		continue;

	    formatAndReport("%S <drop> %O.", item);
	    item->move(pos());
	    myInventory.removeAt(i);

	    total--;
	    if (total <= 15)
		break;
	}
    }
}

void
MOB::addItem(ITEM *item)
{
    int			 i;
    
    // Alert the world to our acquirement.
    if (isAvatar() || pos().isFOV())
	if (item->defn().gaintxt)
	    msg_format(item->defn().gaintxt,
			this, item);

    // First, check to see if we can merge...
    for (i = 0; i < myInventory.entries(); i++)
    {
	if (item->canStackWith(myInventory(i)))
	{
	    myInventory(i)->combineItem(item);
	    delete item;
	    return;
	}
    }

    // Brand new item.
    myInventory.append(item);

    // If we didn't stack, we may need to update our best items.
    updateEquippedItems();
}

ITEM *
MOB::splitStack(ITEM *item)
{
    if (item->getStackCount() > 1)
    {
	ITEM	*result;

	item->decStackCount();
	result = item->createCopy();
	result->setStackCount(1);

	return result;
    }
    else
    {
	removeItem(item);
	return item;
    }
}

void
MOB::removeItem(ITEM *item, bool quiet)
{
    // Alert the world to our acquirement.
    if ((isAvatar() || pos().isFOV()) && !quiet)
	if (item->defn().losetxt)
	    msg_format(item->defn().losetxt,
			this, item);

    myInventory.removePtr(item);

    updateEquippedItems();
}

void
MOB::loseAllItems()
{
    int		i;
    ITEM	*item;

    for (i = myInventory.entries(); i --> 0;)
    {
	item = myInventory(i);
	removeItem(item, true);
	delete item;
    }
}

void
MOB::loseTempItems()
{
    int		i;
    ITEM	*item;

    for (i = myInventory.entries(); i --> 0;)
    {
	item = myInventory(i);
	// All flags and timers are temp.
	if (item->getTimer() >= 0 || item->defn().isflag)
	{
	    removeItem(item, true);
	    delete item;
	}
    }
}

void
MOB::save(ostream &os) const
{
    int		val;
    u8		c;

    val = getDefinition();
    os.write((const char *) &val, sizeof(int));

    myPos.save(os);
    myMeditatePos.save(os);
    myTarget.save(os);
    myHome.save(os);

    os.write((const char *) &myHP, sizeof(int));
    os.write((const char *) &myMP, sizeof(int));
    os.write((const char *) &myAIState, sizeof(int));
    os.write((const char *) &myFleeCount, sizeof(int));
    os.write((const char *) &myBoredom, sizeof(int));
    os.write((const char *) &myYellHystersis, sizeof(int));
    os.write((const char *) &myNumDeaths, sizeof(int));
    os.write((const char *) &myUID, sizeof(int));
    val = myStrategy;
    os.write((const char *) &val, sizeof(int));
    os.write((const char *) &mySearchPower, sizeof(int));

    val = isSwallowed();
    os.write((const char *) &val, sizeof(int));

    int			 numitem;
    int			 i;

    c = mySawMurder;
    os.write((const char *) &c, 1);
    c = mySawMeanMurder;
    os.write((const char *) &c, 1);
    c = mySawVictory;
    os.write((const char *) &c, 1);
    c = myAvatarHasRanged;
    os.write((const char *) &c, 1);

    YELL_NAMES		yell;
    FOREACH_YELL(yell)
    {
	c = myHeardYell[yell];
	os.write((const char *) &c, 1);
	c = myHeardYellSameRoom[yell];
	os.write((const char *) &c, 1);
    }

    numitem = myInventory.entries();
    os.write((const char *) &numitem, sizeof(int));

    for (i = 0; i < myInventory.entries(); i++)
    {
	myInventory(i)->save(os);
    }
}

MOB *
MOB::load(istream &is)
{
    int		 val, num, i;
    u8		 c;
    MOB		*mob;

    mob = new MOB();

    is.read((char *)&val, sizeof(int));
    mob->myDefinition = (MOB_NAMES) val;

    mob->myPos.load(is);
    mob->myMeditatePos.load(is);
    mob->myTarget.load(is);
    mob->myHome.load(is);

    is.read((char *)&mob->myHP, sizeof(int));
    is.read((char *)&mob->myMP, sizeof(int));
    is.read((char *)&mob->myAIState, sizeof(int));
    is.read((char *)&mob->myFleeCount, sizeof(int));
    is.read((char *)&mob->myBoredom, sizeof(int));
    is.read((char *)&mob->myYellHystersis, sizeof(int));
    is.read((char *)&mob->myNumDeaths, sizeof(int));
    is.read((char *)&mob->myUID, sizeof(int));
    is.read((char *)&val, sizeof(int));
    mob->myStrategy = (STRATEGY_STATE) val;
    is.read((char *)&mob->mySearchPower, sizeof(int));
    glb_reportUID(mob->myUID);

    is.read((char *)&val, sizeof(int));
    mob->setSwallowed(val ? true : false);

    is.read((char *)&c, 1);
    mob->mySawMurder = (c ? true : false);
    is.read((char *)&c, 1);
    mob->mySawMeanMurder = (c ? true : false);
    is.read((char *)&c, 1);
    mob->mySawVictory = (c ? true : false);
    is.read((char *)&c, 1);
    mob->myAvatarHasRanged = (c ? true : false);
    
    YELL_NAMES		yell;
    FOREACH_YELL(yell)
    {
	is.read((char *)&c, 1);
	mob->myHeardYell[yell] = (c ? true : false);
	is.read((char *)&c, 1);
	mob->myHeardYellSameRoom[yell] = (c ? true : false);
    }

    is.read((char *)&num, sizeof(int));

    for (i = 0; i < num; i++)
    {
	mob->myInventory.append(ITEM::load(is));
    }

    return mob;
}

bool
MOB::hasVisibleEnemies() const
{
    PTRLIST<MOB *> list;

    getVisibleEnemies(list);
    if (list.entries())
	return true;
    return false;
}

bool
MOB::actionDropSurplus()
{
    ITEM		*weap, *wand;

    weap = lookupWeapon();
    wand = lookupWand();
    for (int i = 0; i < myInventory.entries(); i++)
    {
	if (myInventory(i) == weap)
	    continue;
	if (myInventory(i) == wand)
	    continue;
	if (myInventory(i)->isPotion())
	    continue;

	if (myInventory(i)->defn().isflag)
	    continue;

	return actionDrop(myInventory(i));
    }
    // Check if our wand/weap is redundant
    if (weap && weap->getStackCount() > 1)
    {
	return actionDropButOne(weap);
    }
    if (wand && wand->getStackCount() > 1)
    {
	return actionDropButOne(wand);
    }
    return false;
}

bool
MOB::hasSurplusItems() const
{
    bool	hasrange = false;
    bool	hasmelee = false;

    for (int i = 0; i < myInventory.entries(); i++)
    {
	// We can always drink these.
	if (myInventory(i)->isPotion())
	    continue;
	if (myInventory(i)->defn().isflag)
	    continue;
	if (myInventory(i)->isRanged())
	{
	    if (myInventory(i)->getStackCount() > 1)
		return true;
	    if (hasrange)
		return true;
	    hasrange = true;
	    continue;
	}
	if (myInventory(i)->isMelee())
	{
	    if (myInventory(i)->getStackCount() > 1)
		return true;
	    if (hasmelee)
		return true;
	    hasmelee = true;
	    continue;
	}
	return true;
    }

    return false;
}

int
MOB::numSurplusRange() const
{
    int		numrange = 0;

    for (int i = 0; i < myInventory.entries(); i++)
    {
	if (myInventory(i)->isRanged())
	{
	    numrange += myInventory(i)->getStackCount();
	}
    }

    // Reserve one range for our own use.
    return numrange - 1;
}

void
MOB::getVisibleEnemies(PTRLIST<MOB *> &list) const
{
    // If we are blind, we can't see anyone.
    if (hasItem(ITEM_BLIND))
	return;

    // We only care about the non-avatar case now.
    if (isAvatar())
	return;

    // Check if we are in FOV.  If so, the avatar is visible.
    if (getAvatar())
    {
	// Ignore dead avatars.
	if (!getAvatar()->alive())
	{
	    return;
	}
	// Ignore none hostile
	if (isFriends(getAvatar()))
	    return;

	// We need the double check because if you are meditating
	// and put them in the fov, that doesn't mean that 
	// the avatar is now in fov.
	if (pos().isFOV() && getAvatar()->pos().isFOV())
	{
	    list.append(getAvatar());
	}
    }
}

void
MOB::formatAndReport(const char *msg)
{
    if (isAvatar() || pos().isFOV())
    {
	msg_format(msg, this);
    }
}

void
MOB::formatAndReport(const char *msg, MOB *object)
{
    if (isAvatar() || pos().isFOV())
    {
	msg_format(msg, this, object);
    }
}

void
MOB::formatAndReport(const char *msg, ITEM *object)
{
    if (isAvatar() || pos().isFOV())
    {
	msg_format(msg, this, object);
    }
}

void
MOB::formatAndReport(const char *msg, const char *object)
{
    if (isAvatar() || pos().isFOV())
    {
	msg_format(msg, this, object);
    }
}

int
MOB::numberMeleeHostiles() const
{
    int		dx, dy;
    MOB		*mob;
    int		hostiles = 0;

    FORALL_8DIR(dx, dy)
    {
	mob = pos().delta(dx, dy).mob();

	if (mob && !mob->isFriends(this))
	    hostiles++;
    }
    return hostiles;
}

void
MOB::clearCollision()
{
    myCollisionSources.clear();
    myCollisionTarget = 0;
}

void
MOB::setCollisionTarget(MOB *target)
{
    if (myCollisionTarget == target)
	return;

    assert(!target || !myCollisionTarget);

    myCollisionTarget = target;
    if (target)
	target->collisionSources().append(this);
}

bool
MOB::actionPortalFire(int dx, int dy, int portal)
{
    // No suicide.
    if (!dx && !dy)
    {
	msg_format("%S <decide> not to aim at %O.", this, this);
	return false;
    }
    
    // If swallowed, rather useless.
    if (isSwallowed())
    {
	msg_format("%S do not have enough room inside here.", this);
	return false;
    }

    clearBoredom();

    // Always display now.
 
    u8		symbol = '*';
    ATTR_NAMES	attr;
    int		portalrange = 60;

    attr = ATTR_BLUE;
    if (portal)
	attr = ATTR_ORANGE;
    
    pos().displayBullet(portalrange,
			dx, dy,
			symbol, attr,
			false);

    POS		vpos;
    vpos = pos().traceBulletPos(portalrange, dx, dy, false, false);
    
    if (vpos.tile() != TILE_WALL && vpos.tile() != TILE_PROTOPORTAL)
    {
	msg_format("The portal dissipates at %O.", 0, vpos.defn().legend);
	return false;
    }

    if (!glb_roomtypedefs[vpos.roomType()].allowportal)
    {
	formatAndReport("The walls of this area seem unable to hold portals.");
	return false;
    }


    // We want sloppy targeting for portals.
    // This means that given a portal location of # fired at from the south,
    // we want:
    //
    // 1#1
    // 2*2
    //
    // as potential portals.
    //
    // One fired from the south-west
    //  2
    // 1#2
    // *1

    POS		alt[5];

    if (dx && dy)
    {
	alt[0] = vpos;
	alt[1] = vpos.delta(-dx, 0);
	alt[2] = vpos.delta(0, -dy);
	alt[3] = vpos.delta(0, dy);
	alt[4] = vpos.delta(dx, 0);
    }
    else if (dx)
    {
	alt[0] = vpos;
	alt[1] = vpos.delta(0, 1);
	alt[2] = vpos.delta(0, -1);
	alt[3] = vpos.delta(-dx, 1);
	alt[4] = vpos.delta(-dx, -1);
    }
    else
    {
	alt[0] = vpos;
	alt[1] = vpos.delta(1, 0);
	alt[2] = vpos.delta(-1, 0);
	alt[3] = vpos.delta(1, -dy);
	alt[4] = vpos.delta(-1, -dy);
    }

    for (int i = 0; i < 5; i++)
    {
	if (buildPortalAtLocation(alt[i], portal))
	    return true;
    }
    msg_report("The wall proved too unstable to hold a portal.");

    return false;
}

bool
MOB::buildPortalAtLocation(POS vpos, int portal) const
{
    int		origangle;

    origangle = vpos.angle();

    // We want all our portals in the same space.
    vpos.setAngle(0);

    // Check if it is a valid portal pos?
    if (!vpos.prepSquareForDestruction())
    {
	return false;
    }

    // Verify the portal is well formed, ie, three neighbours are now
    // walls and the other is a floor.
    int		dir, floordir = -1;
    TILE_NAMES	tile;

    for (dir = 0; dir < 4; dir++)
    {
	tile = vpos.delta4Direction(dir).tile();
	if (tile == TILE_FLOOR)
	{
	    if (floordir < 0)
		floordir = dir;
	    else
	    {
		// Uh oh.
		return false;

	    }
	}
	else if (tile == TILE_WALL || tile == TILE_PROTOPORTAL)
	{
	    // All good.
	}
	else
	{
	    // Uh oh.
	    return false;
	}
    }

    // Failed to find a proper portal.
    if (floordir < 0)
	return false;

    // In floordir+2 we will be placing the mirror of the opposite
    // portal.  It is important that square is not accessible.  We
    // merely make sure it isn't a floor
    // Still an issue of having ants dig it out.  Ideally we'd
    // have the virtual portal flagged with MAPFLAG_PORTAL but
    // we don't do that currently in buildPortal and doing so
    // would mean we'd have to clean it up properly.

    POS		virtualportal;

    virtualportal = vpos.delta4Direction((floordir+2)&3);
    virtualportal = virtualportal.delta4Direction((floordir+2)&3);

    // We now point to the square behind the proposed virtual portal
    // this should be wall or invalid
    tile = virtualportal.tile();
    if (tile != TILE_WALL && tile != TILE_PROTOPORTAL && tile != TILE_INVALID)
	return false;
    // Try neighbours.
    tile = virtualportal.delta4Direction((floordir+1)&3).tile();
    if (tile != TILE_WALL && tile != TILE_PROTOPORTAL && tile != TILE_INVALID)
	return false;
    tile = virtualportal.delta4Direction((floordir-1)&3).tile();
    if (tile != TILE_WALL && tile != TILE_PROTOPORTAL && tile != TILE_INVALID)
	return false;

    // We now know we aren't an isolated island.  Yet.

    vpos.map()->buildUserPortal(vpos, portal, (floordir+2) & 3, origangle);

    return true;
}

