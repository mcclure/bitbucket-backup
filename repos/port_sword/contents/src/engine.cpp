/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        engine.cpp ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 *	Our game engine.  Grabs commands from its command
 *	queue and processes them as fast as possible.  Provides
 *	a mutexed method to get a recent map from other threads.
 *	Note that this will run on its own thread!
 */

#include <libtcod.hpp>

#include "engine.h"

#include "msg.h"
#include "rand.h"
#include "map.h"
#include "mob.h"
#include "speed.h"
#include "item.h"
#include "text.h"
#include "strategy.h"
#include <time.h>

#include <fstream>
using namespace std;

// #define DO_TIMING

#define STRAT_START_TIME	24
#define STRAT_QUEST_TIME	36

static void *
threadstarter(void *vdata)
{
    ENGINE	*engine = (ENGINE *) vdata;

    engine->mainLoop();

    return 0;
}

static void *
strategystarter(void *vdata)
{
    ENGINE	*engine = (ENGINE *) vdata;

    engine->strategyLoop();

    return 0;
}

ENGINE::ENGINE(DISPLAY *display)
{
    myMap = myOldMap = 0;
    myDisplay = display;
    myStrategy = myOldStrategy = 0;
    myPendingStrategyUpdate = false;

    myThread = THREAD::alloc();
    myThread->start(threadstarter, this);

    myStrategyThread = THREAD::alloc();
    myStrategyThread->start(strategystarter, this);
}

ENGINE::~ENGINE()
{
    if (myMap)
	myMap->decRef();
    if (myOldMap)
	myOldMap->decRef();
    if (myStrategy)
	myStrategy->decRef();
    if (myOldStrategy)
	myOldStrategy->decRef();
}

MAP *
ENGINE::copyMap()
{
    AUTOLOCK	a(myMapLock);

    if (myOldMap)
	myOldMap->incRef();

    return myOldMap;
}

void
ENGINE::updateMap()
{
    {
	AUTOLOCK	a(myMapLock);

	if (myOldMap)
	    myOldMap->decRef();

	myOldMap = myMap;
    }
    if (myOldMap)
    {
#ifdef DO_TIMING
    int 		movestart = TCOD_sys_elapsed_milli();
#endif
	myMap = new MAP(*myOldMap);
	myMap->incRef();
#ifdef DO_TIMING
    int 		moveend = TCOD_sys_elapsed_milli();
    cerr << "Map update " << moveend - movestart << endl;
#endif
    }
}

STRATEGY *
ENGINE::copyStrategy()
{
    // We do not need the live lock as we arne't affected by it!
    AUTOLOCK	a(myStrategyLock);

    if (myOldStrategy)
	myOldStrategy->incRef();

    return myOldStrategy;
}

void
ENGINE::updateStrategy()
{
    AUTOLOCK	ab(myLiveStrategyLock);
    {
	AUTOLOCK	a(myStrategyLock);

	if (myOldStrategy)
	    myOldStrategy->decRef();

	myOldStrategy = myStrategy;
    }
    if (myOldStrategy)
    {
#ifdef DO_TIMING
    int 		movestart = TCOD_sys_elapsed_milli();
#endif
	myStrategy = new STRATEGY(*myOldStrategy);
	myStrategy->incRef();
#ifdef DO_TIMING
    int 		moveend = TCOD_sys_elapsed_milli();
    cerr << "Strategy update " << moveend - movestart << endl;
#endif
    }
}

#define VERIFY_ALIVE() \
    if (avatar && !avatar->alive()) \
    {			\
	msg_report("Dead people can't move.  ");	\
	break;			\
    }

void redrawWorld();

bool
ENGINE::awaitRebuild()
{
    int		value = 0;

    while (!myRebuildQueue.remove(value))
    {
	redrawWorld();
    }

    myRebuildQueue.clear();

    return value ? true : false;
}

void
ENGINE::awaitStrategyUpdate()
{
    int		value = 0;

    if (!myPendingStrategyUpdate)
	return;

    while (!myStrategyUpdateQueue.remove(value))
    {
	redrawWorld();
    }
    myPendingStrategyUpdate = false;
    myStrategyUpdateQueue.clear();
}

void
ENGINE::awaitSave()
{
    int		value;

    while (!mySaveQueue.remove(value))
    {
	redrawWorld();
    }

    mySaveQueue.clear();
}

void
ENGINE::integrateStrategy(QUEST_NAMES quest, int turns)
{
    STRATCOMMAND	cmd;
    cmd.turns = turns;
    cmd.quest = quest;
    myPendingStrategyUpdate = true;
    myStrategyQueue.append(cmd);
}

BUF
ENGINE::getNextPopup()
{
    BUF		result;

    myPopupQueue.remove(result);

    return result;
}

void
ENGINE::popupText(const char *text)
{
    BUF		buf;
    
    buf.strcpy(text);

    myPopupQueue.append(buf);
}

MOB_NAMES
ENGINE::getNextShopper()
{
    MOB_NAMES	shopper = MOB_NONE;

    myShopQueue.remove(shopper);

    return shopper;
}

void
ENGINE::shopRequest(MOB_NAMES mob)
{
    myShopQueue.append(mob);
}

void
ENGINE::mainLoop()
{
    rand_setseed((long) time(0));

    COMMAND		cmd;
    MOB			*avatar;
    bool		timeused = false;
    bool		doheartbeat = true;

    while (1)
    {
	avatar = 0;
	if (myMap)
	    avatar = myMap->avatar();

	timeused = false;
	if (doheartbeat && avatar && avatar->aiForcedAction())
	{
	    cmd = COMMAND(ACTION_NONE);
	    timeused = true;
	}
	else
	{
	    if (doheartbeat)
		msg_newturn();

	    // Allow the other thread a chance to redraw.
	    // Possible we might want a delay here?
	    updateMap();
	    avatar = 0;
	    if (myMap)
		avatar = myMap->avatar();
	    cmd = queue().waitAndRemove();

	    doheartbeat = false;
	}

	switch (cmd.action())
	{
	    case ACTION_WAIT:
		// The dead are allowed to wait!
		timeused = true;
		break;

	    case ACTION_WORLDSTATE:
	    {
		WORLDSTATE_NAMES	state = (WORLDSTATE_NAMES) cmd.dx();

		glbWorldState = state;

		if (state == WORLDSTATE_LOST)
		{
		    fadeFromWhite();

		    if (myMap)
		    {
			myMap->killAllButAvatar();
			myMap->lootAllValuable();
		    }
		}
		break;
	    }

	    case ACTION_STARTQUEST:
	    {
		QUEST_NAMES	quest = (QUEST_NAMES) cmd.dx();

		if (myMap)
		    myMap->decRef();

		glbCurrentQuest = quest;
		glbWorldState = WORLDSTATE_QUEST;

		popupText(text_lookup("quest", glb_questdefs[quest].descr, "begin"));

		ITEM::initSystem();
		avatar = MOB::create(MOB_AVATAR);
		myMap = new MAP((ATLAS_NAMES)glb_questdefs[quest].atlas, avatar, myDisplay);

		myMap->incRef();
		myMap->setDisplay(myDisplay);
		myMap->rebuildFOV();
		myMap->cacheItemPos();

		break;
	    }

	    case ACTION_ENDQUEST:
	    {
		QUEST_NAMES	quest = (QUEST_NAMES) cmd.dx();

		// Really, the quest is supposed to be our global
		// quest but, no matter!
		integrateStrategy(quest, STRAT_QUEST_TIME);

		if (myMap)
		    myMap->decRef();

		popupText(text_lookup("quest", glb_questdefs[quest].descr, "done"));
		glbLastQuest = quest;
		glbCurrentQuest = QUEST_NONE;
		glbWorldState = WORLDSTATE_ENDQUEST;
		glbLevel++;

		ITEM::initSystem();
		avatar = MOB::create(MOB_AVATAR);
		myMap = new MAP(ATLAS_THRONE, avatar, myDisplay);

		myMap->incRef();
		myMap->setDisplay(myDisplay);
		myMap->rebuildFOV();
		myMap->cacheItemPos();
		break;
	    }

	    case ACTION_RESTART:
	    {
		int		loaded = 1;
		AUTOLOCK	a(myLiveStrategyLock);

		if (myMap)
		    myMap->decRef();
		if (myStrategy)
		    myStrategy->decRef();

		glbCurrentQuest = QUEST_NONE;
		glbLastQuest = QUEST_NONE;
		glbWorldState = WORLDSTATE_CHOOSEQUEST;
		glbLevel = 1;
		loaded = load();
		if (!loaded)
		{
		    loaded = 0;
		    ITEM::initSystem();
		    avatar = MOB::create(MOB_AVATAR);
		    myMap = new MAP((ATLAS_NAMES)cmd.dx(), avatar, myDisplay);
		    myStrategy = new STRATEGY(ATLAS_STRATEGY);
		    myStrategyQueue.clear();

		    integrateStrategy(QUEST_NONE, STRAT_START_TIME);
		}

		myMap->incRef();
		myStrategy->incRef();
		myMap->setDisplay(myDisplay);
		myMap->rebuildFOV();
		myMap->cacheItemPos();

		// Our new strategy needs to replace what was there.
		updateStrategy();

		// Flag we've rebuilt.
		myRebuildQueue.append(loaded);
		break;
	    }

	    case ACTION_SAVE:
	    {
		// This also ensures our strategy is up to date
		// prior to saving.
		AUTOLOCK	a(myLiveStrategyLock);

		// Only save good games
		if (myMap && avatar && avatar->alive())
		{
		    save();
		}
		mySaveQueue.append(0);
		break;
	    }

	    case ACTION_REBOOTAVATAR:
	    {
		if (avatar)
		    avatar->gainHP(avatar->defn().max_hp);
		break;
	    }

	    case ACTION_BUMP:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionBump(cmd.dx(), cmd.dy());
		if (!timeused)
		    msg_newturn();

		break;
	    }
	
	    case ACTION_DROP:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionDrop(avatar->getItemFromNo(cmd.dx()));
		if (!timeused)
		    msg_newturn();
		break;
	    }
	
	    case ACTION_BREAK:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionBreak(avatar->getItemFromNo(cmd.dx()));
		if (!timeused)
		    msg_newturn();
		break;
	    }
	
	    case ACTION_WEAR:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionWear(avatar->getItemFromNo(cmd.dx()));
		if (!timeused)
		    msg_newturn();
		break;
	    }
	
	    case ACTION_PICKUP:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionPickup();
		if (!timeused)
		    msg_newturn();
		break;
	    }
	
	    case ACTION_EAT:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionEat(avatar->getItemFromNo(cmd.dx()));
		if (!timeused)
		    msg_newturn();
		break;
	    }
	
	    case ACTION_MEDITATE:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionMeditate();
		if (!timeused)
		    msg_newturn();
		break;
	    }
	
	    case ACTION_SEARCH:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionSearch();
		if (!timeused)
		    msg_newturn();
		break;
	    }

	    case ACTION_CREATEITEM:
	    {
		VERIFY_ALIVE()
		if (avatar)
		{
		    ITEM	*item = ITEM::create((ITEM_NAMES) cmd.dx());
		    avatar->addItem(item);
		}
		break;
	    }
	
	    case ACTION_QUAFF:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionQuaff(avatar->getItemFromNo(cmd.dx()));
		if (!timeused)
		    msg_newturn();
		break;
	    }

	    case ACTION_THROW:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionThrow(avatar->getItemFromNo(cmd.dz()),
					cmd.dx(), cmd.dy());
		if (!timeused)
		    msg_newturn();
		break;
	    }

	    case ACTION_CAST:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionCast((SPELL_NAMES)cmd.dz(),
					cmd.dx(), cmd.dy());
		if (!timeused)
		    msg_newturn();
		break;
	    }
	
	    case ACTION_SHOP:
	    {
		VERIFY_ALIVE()
		if (avatar)
		    timeused = avatar->actionShop((SHOP_NAMES)cmd.dx(),
					cmd.dy(), cmd.dz());
		if (!timeused)
		    msg_newturn();
		break;
	    }
	
	    case ACTION_ROTATE:
	    {
		if (avatar)
		    timeused = avatar->actionRotate(cmd.dx());
		break;
	    }

	    case ACTION_FIRE:
	    {
		VERIFY_ALIVE()
		if (avatar)
		{
		    timeused = avatar->actionFire(cmd.dx(), cmd.dy());
		}
		if (!timeused)
		    msg_newturn();
		break;
	    }

	    case ACTION_MAGICMOVE:
	    {
		VERIFY_ALIVE()
		if (avatar)
		{
		    timeused = avatar->actionMagicMove((MAGICMOVE_NAMES) cmd.dz(), cmd.dx(), cmd.dy());
		}
		if (!timeused)
		    msg_newturn();
		break;
	    }

	    case ACTION_SUICIDE:
	    {
		if (avatar)
		{
		    if (avatar->alive())
		    {
			msg_report("Your time has run out!  ");
			// We want the flame to die.
			avatar->gainHP(-avatar->getHP());
		    }
		}
		break;
	    }
	}

	if (myMap && timeused)
	{
	    if (cmd.action() != ACTION_SEARCH &&
		cmd.action() != ACTION_NONE)
	    {
		// Depower searching
		if (avatar)
		    avatar->setSearchPower(0);
	    }

	    // We need to build the FOV for the monsters as they
	    // rely on the updated FOV to track, etc.
	    // Rebuild the FOV map
	    // Don't do this if no avatar, or the avatar is dead,
	    // as we want the old fov.
	    if (avatar && avatar->alive())
		myMap->rebuildFOV();

	    // Update the world.
	    myMap->doMoveNPC();
	    spd_inctime();

	    // Rebuild the FOV map
	    myMap->rebuildFOV();

	    doheartbeat = true;
	}

	// Allow the other thread a chance to redraw.
	// Possible we might want a delay here?
	updateMap();
    }
}

void
ENGINE::strategyLoop()
{
    while (1)
    {
	STRATCOMMAND	cmd;
	int		turnuse;
	bool		live = true;
	cmd = myStrategyQueue.waitAndRemove();

	turnuse = cmd.turns;
	QUEST_NAMES	quest = cmd.quest;

	// No one can do anything until we complete our strategy.
	AUTOLOCK	a(myLiveStrategyLock);

	// Grant the appropriate powers...
	myStrategy->grantQuestBonus(0, quest);

	for (int i = 1; i < turnuse; i++)
	{
	    // Update strategy...
	    live = myStrategy->integrate();
	    if (!live)
		break;
	}

	// And now the foe learns of it!
	if (live)
	{
	    myStrategy->grantQuestBonus(1, quest);
	    myStrategy->integrate();
	}

	// Write back result.
	updateStrategy();

	// Wake up any sleepers.
	myStrategyUpdateQueue.append(1);
    }
}

bool
ENGINE::load()
{
    // Did I mention my hatred of streams?
    {
#ifdef WIN32
	ifstream	is("sword.sav", ios::in | ios::binary);
#else
	ifstream	is("sword.sav");
#endif

	if (!is)
	    return false;

	myMap = new MAP(is);
	myStrategy = STRATEGY::load(is);
    }

    // Scope out the stream so we can have it unlocked to delete.
    ::unlink("sword.sav");

    return true;
}

void
ENGINE::save()
{
    // Did I mention my hatred of streams?
#ifdef WIN32
    ofstream	os("sword.sav", ios::out | ios::binary);
#else
    ofstream	os("sword.sav");
#endif

    myMap->save(os);
    myStrategy->save(os);
}
