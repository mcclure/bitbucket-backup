/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        engine.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 *	Our game engine.  Grabs commands from its command
 *	queue and processes them as fast as possible.  Provides
 *	a mutexed method to get a recent map from other threads.
 *	Note that this will run on its own thread!
 */

#ifndef __engine__
#define __engine__

#include "command.h"
#include "thread.h"
#include "display.h"
#include "buf.h"

class THREAD;
class MAP;
class DISPLAY;
class MOB;
class STRATEGY;

class ENGINE
{
public:
    ENGINE(DISPLAY *display);
    ~ENGINE();

    // These methods are thread safe.
    CMD_QUEUE		&queue() { return myQueue; }
    // Returns a map with the reference copy incremented, you must
    // decRef when done!
    MAP			*copyMap();

    // Returns a strategy with the reference copy incremented, you must
    // decref when done
    STRATEGY		*copyStrategy();

    // Public only for call back conveninece.
    void		 mainLoop();
    // For the stragetic layer so it can run in parallel with movement
    // as the person returns to the map table.
    void		 strategyLoop();

    // Waits for the map to finish building, invoke after
    // a RESTART command.
    // Returns true if loaded from disk
    bool		 awaitRebuild();

    // Update the strategy map for a certain number of years
    void		 integrateStrategy(QUEST_NAMES quest, int turns);

    // Waits for the strategy integration to end, returns immediately
    // if nothing is pending.
    void		 awaitStrategyUpdate();

    // Waits for the map to finish saving after ACTION_SAVE
    void		 awaitSave();

    // Returns the next requested popup notifier.
    // Returns empty string if none.
    BUF			 getNextPopup();

    // Call from any thread, triggers a delayed popup
    void		 popupText(const char *text);
    void		 popupText(BUF buf) { popupText(buf.buffer()); }

    MOB_NAMES		 getNextShopper();
    void		 shopRequest(MOB_NAMES mob);

    void		 fadeFromWhite() { myDisplay->fadeFromWhite(); }

protected:
    /// Called when we are happy with myMap and want to publish it.
    void		 updateMap();
    void		 updateStrategy();

    bool		 load();
    void		 save();

private:
    MAP			*myMap;
    MAP			*myOldMap;
    LOCK		 myMapLock;

    // Updated on which thread?
    STRATEGY		*myStrategy;
    STRATEGY		*myOldStrategy;
    // A lock on the external old strategy.
    LOCK		 myStrategyLock;
    // A lock inside of engine if myStrategy is being updated by engine
    // like things.
    LOCK		 myLiveStrategyLock;

    CMD_QUEUE		 myQueue;
    DISPLAY		*myDisplay;

    QUEUE<int>		 myRebuildQueue;
    QUEUE<int>		 mySaveQueue;

    struct STRATCOMMAND
    {
	STRATCOMMAND() {}
	STRATCOMMAND(int turns) { quest = QUEST_NONE; turns = turns; }
	QUEST_NAMES	quest;
	int		turns;
    };
    QUEUE<STRATCOMMAND>	 myStrategyQueue;
    bool		 myPendingStrategyUpdate;
    QUEUE<int>		 myStrategyUpdateQueue;

    // TODO: THIS IS A MEMORY LEAK
    QUEUE<BUF>		 myPopupQueue;

    QUEUE<MOB_NAMES>	 myShopQueue;

    THREAD		*myThread;
    THREAD		*myStrategyThread;
};

extern ENGINE *glbEngine;

#endif
