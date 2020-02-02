/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        main.cpp ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 */

#include <libtcod.hpp>
#include <stdio.h>
#include <time.h>

#ifdef __APPLE__
// For main() magic
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

//#define USE_AUDIO

#define POPUP_DELAY 1000
//#define POPUP_DELAY 10

#ifdef WIN32
#include <windows.h>
#include <process.h>
#define usleep(x) Sleep((x)*1000)
#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "SDLmain.lib")

// I really hate windows.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#else
#include <unistd.h>
#endif

#include <fstream>
using namespace std;

int		 glbLevelStartMS = -1;

#include "rand.h"
#include "gfxengine.h"
#include "config.h"
#include "civstate.h"

#ifdef USE_AUDIO
#include <SDL_mixer.h>

Mix_Music		*glbTunes = 0;
#endif
bool			 glbMusicActive = false;

// Cannot be bool!
// libtcod does not accept bools properly.

volatile int		glbItemCache = -1;

SPELL_NAMES		glbLastSpell = SPELL_NONE;
int			glbLastItemIdx = 0;
int			glbLastItemAction = 0;

bool reloadLevel(bool );

void endOfMusic()
{
    // Keep playing.
    // glbMusicActive = false;
}

void
setMusicVolume(int volume)
{
#ifdef USE_AUDIO
    volume = BOUND(volume, 0, 10);

    glbConfig->myMusicVolume = volume;

    Mix_VolumeMusic((MIX_MAX_VOLUME * volume) / 10);
#endif
}

void
stopMusic(bool atoncedamnit = false)
{
#ifdef USE_AUDIO
    if (glbMusicActive)
    {
	if (atoncedamnit)
	    Mix_HaltMusic();
	else
	    Mix_FadeOutMusic(2000);
	glbMusicActive = false;
    }
#endif
}

void
startMusic()
{
#ifdef USE_AUDIO
    if (glbMusicActive)
	stopMusic();

    if (!glbTunes)
	glbTunes = Mix_LoadMUS(glbConfig->musicFile());
    if (!glbTunes)
    {
	printf("Failed to load %s, error %s\n", 
		glbConfig->musicFile(), 
		Mix_GetError());
    }
    else
    {
	glbMusicActive = true;
	glbLevelStartMS = TCOD_sys_elapsed_milli();

	if (glbConfig->musicEnable())
	{
	    Mix_PlayMusic(glbTunes, -1);		// Infinite
	    Mix_HookMusicFinished(endOfMusic);

	    setMusicVolume(glbConfig->myMusicVolume);
	}
    }
#endif
}


#ifdef main
#undef main
#endif

#include "mob.h"
#include "map.h"
#include "text.h"
#include "speed.h"
#include "display.h"
#include "engine.h"
#include "panel.h"
#include "msg.h"
#include "firefly.h"
#include "item.h"
#include "chooser.h"
#include "strageticview.h"

PANEL		*glbMessagePanel = 0;
DISPLAY		*glbDisplay = 0;
MAP		*glbMap = 0;
ENGINE		*glbEngine = 0;
PANEL		*glbPopUp = 0;
PANEL		*glbJacob = 0;
PANEL		*glbRHSBar = 0;
PANEL		*glbWeaponInfo = 0;
PANEL		*glbVictoryInfo = 0;
bool		 glbPopUpActive = false;
bool		 glbRoleActive = false;
FIREFLY		*glbHealthFire = 0;
bool		 glbVeryFirstRun = true;

STRAGETICVIEW	*glbStrategyView;
bool		 glbStrategyViewActive = false;

CHOOSER		*glbLevelBuildStatus = 0;

CHOOSER		*glbChooser = 0;
bool		 glbChooserActive = false;

int		 glbInvertX = -1, glbInvertY = -1;
int		 glbMeditateX = -1, glbMeditateY = -1;

#define DEATHTIME 15000


BUF
mstotime(int ms)
{
    int		sec, min;
    BUF		buf;

    sec = ms / 1000;
    ms -= sec * 1000;
    min = sec / 60;
    sec -= min * 60;

    if (min)
	buf.sprintf("%dm%d.%03ds", min, sec, ms);
    else if (sec)
	buf.sprintf("%d.%03ds", sec, ms);
    else
	buf.sprintf("0.%03ds", ms);

    return buf;
}

MAGICMOVE_NAMES
getMoveFromKey(u8 key)
{
    MAGICMOVE_NAMES		move;
    FOREACH_MAGICMOVE(move)
    {
	if (move == MAGICMOVE_NONE)
	    continue;
	if (key == glb_magicmovedefs[move].key)
	{
	    return move;
	}
    }
    return MAGICMOVE_NONE;
}

void
displayMagicMoves()
{
    if (glbStrategyViewActive)
	return;

    glbRHSBar->clear();
    MAGICMOVE_NAMES		move;

    FOREACH_MAGICMOVE(move)
    {
	int		i;

	if (move == MAGICMOVE_NONE)
	    continue;

	glbRHSBar->newLine();
	glbRHSBar->setTextAttr(ATTR_WHITE);
	BUF		buf;
	buf.sprintf("%c %s\n", glb_magicmovedefs[move].key, glb_magicmovedefs[move].name);
	glbRHSBar->appendText(buf);

	glbRHSBar->appendText("  ");
	glbRHSBar->setTextAttr(ATTR_RED);
	for (i = 0; i < glb_magicmovedefs[move].blood; i++)
	    glbRHSBar->appendText("O");
	for ( ; i < 7 - glb_magicmovedefs[move].damage; i++)
	    glbRHSBar->appendText(" ");

	glbRHSBar->setTextAttr(ATTR_YELLOW);
	for (i = 0; i < glb_magicmovedefs[move].damage; i++)
	    glbRHSBar->appendText("X");
	glbRHSBar->newLine();

	const char *pattern = glb_magicmovedefs[move].pattern;

	for (int y = 0; y < 5; y++)
	{
	    glbRHSBar->appendText("  ");
	    for (int x = 0; x < 7; x++)
	    {
		ATTR_NAMES		attr = ATTR_NORMAL;
		u8			sym = *pattern;
		char			s[2] = { 0, 0 };
		switch (*pattern)
		{
		    case '*':
			attr = ATTR_RED;
			break;
		    case '@':
			attr = ATTR_WHITE;
			break;
		    case 'o':
			attr = ATTR_OUTOFFOV;
			sym = '@';
			break;
		    case '!':
			attr = ATTR_YELLOW;
			sym = '*';
			break;
		}
		glbRHSBar->setTextAttr(attr);
		s[0] = sym;
		glbRHSBar->appendText(s);
		pattern++;
	    }
	    glbRHSBar->newLine();
	}

    }
}

// Should call this in all of our loops.
void
redrawWorld()
{
    static MAP	*centermap = 0;
    static POS	 mapcenter;
    bool	 isblind = false;
    bool	 isvictory = false;

    if (glbWorldState == WORLDSTATE_WON ||
	glbWorldState == WORLDSTATE_LOST)
    {
	isvictory = true;
    }

    gfx_updatepulsetime();

    // Wipe the world
    for (int y = 0; y < SCR_HEIGHT; y++)
	for (int x = 0; x < SCR_WIDTH; x++)
	    gfx_printchar(x, y, ' ');

    if (glbMap)
	glbMap->decRef();
    glbMap = glbEngine->copyMap();

    glbWeaponInfo->clear();

    glbMeditateX = -1;
    glbMeditateY = -1;
    int			avataralive = 0;
    if (glbMap && glbMap->avatar())
    {
	MOB		*avatar = glbMap->avatar();
	int		 timems;

	avataralive = avatar->alive();

	if (avatar->alive() && avatar->isMeditating())
	{
	    glbMeditateX = glbDisplay->x() + glbDisplay->width()/2;
	    glbMeditateY = glbDisplay->y() + glbDisplay->height()/2;
	}

	timems = TCOD_sys_elapsed_milli();

	if (centermap)
	    centermap->decRef();
	centermap = glbMap;
	centermap->incRef();
	mapcenter = avatar->meditatePos();

	isblind = avatar->hasItem(ITEM_BLIND);

	if (avatar->hasItem(ITEM_INVULNERABLE))
	    glbHealthFire->setFireType(FIRE_MONO);
	else if (avatar->hasItem(ITEM_POISON))
	    glbHealthFire->setFireType(FIRE_POISON);
	else
	    glbHealthFire->setFireType(FIRE_BLOOD);

	glbHealthFire->setRatioLiving(avatar->getHP() / (float) avatar->defn().max_hp);

	// Pass on the birth deaths...
	int		partchange;
	while (glbDisplay->popHealthChange(partchange))
	    glbHealthFire->postParticleChange(partchange);

	glbHealthFire->setParticleCount(avatar->getHP());
    }
    
    glbDisplay->display(mapcenter, isblind);
    glbMessagePanel->redraw();

    glbJacob->clear();
    glbJacob->setTextAttr(ATTR_WHITE);
    BUF		buf;

    buf.sprintf("Location: %s", glbMap ? glb_atlasdefs[glbMap->getAtlas()].name : "Limbo");
    glbJacob->appendText(buf);

    glbJacob->redraw();

    displayMagicMoves();

    glbRHSBar->redraw();

    glbWeaponInfo->redraw();

    if (isvictory)
    {
	glbVictoryInfo->clear();
	glbVictoryInfo->appendText("Press 'v' to declare victory.");
	glbVictoryInfo->redraw();
    }

    {
	FIRETEX		*tex = glbHealthFire->getTex();
	tex->redraw(0, 1);
	tex->decRef();
    }
    {
	// we have to clear the RHS.
	static FIRETEX		 *cleartex = 0;
	if (!cleartex)
	{
	    cleartex = new FIRETEX(20, 48);
	    cleartex->buildFromConstant(false, 0, FIRE_HEALTH);
	}
	cleartex->redraw(60, 2);
    }

    if (glbMeditateX >= 0 && glbMeditateY >= 0)
    {
	gfx_printattrback(glbMeditateX, glbMeditateY, ATTR_AVATARMEDITATE);
    }

    if (glbInvertX >= 0 && glbInvertY >= 0)
    {
	gfx_printattr(glbInvertX, glbInvertY, ATTR_HILITE);
    }

    if (glbStrategyViewActive)
    {
	glbStrategyView->display();
    }


    if (glbItemCache >= 0)
    {
	glbLevelBuildStatus->clear();

	glbLevelBuildStatus->appendChoice("   Building Distance Caches   ", glbItemCache);
	glbLevelBuildStatus->redraw();
    }

    if (glbChooserActive)
    {
	glbChooser->redraw();
    }

    glbDisplay->redrawFade();

    if (glbPopUpActive)
	glbPopUp->redraw();

    TCODConsole::flush();
}

// If you have an action which you are waiting for, you can
// use this popup prior to the timeout to ensure the user is warned.
// You should rather use the popupText though!
void
popupTextOpen(const char *buf)
{
    int		key = 0;
    int		startms = TCOD_sys_elapsed_milli();

    glbPopUp->clear();
    glbPopUp->resize(50, 48);
    glbPopUp->move(15, 1);

    // A header matches the footer.
    glbPopUp->appendText("\n");

    // Must make active first because append text may trigger a 
    // redraw if it is long!
    glbPopUpActive = true;
    glbPopUp->appendText(buf);

    // In case we drew due to too long text
    glbPopUp->erase();

    // Determine the size of the popup and resize.
    glbPopUp->shrinkToFit();
}

void
popupTextClose()
{
    glbPopUpActive = false;
    glbPopUp->erase();

    glbPopUp->resize(50, 30);
    glbPopUp->move(15, 10);
}

// Pops up the given text onto the screen.
// The key used to dismiss the text is the return code.
int
popupText(const char *buf, int delayms = 0)
{
    int		key = 0;
    int		startms = TCOD_sys_elapsed_milli();

    glbPopUp->clear();
    glbPopUp->resize(50, 48);
    glbPopUp->move(15, 1);

    // A header matches the footer.
    glbPopUp->appendText("\n");

    // Must make active first because append text may trigger a 
    // redraw if it is long!
    glbPopUpActive = true;
    glbPopUp->appendText(buf);

    // In case we drew due to too long text
    glbPopUp->erase();

    // Determine the size of the popup and resize.
    glbPopUp->shrinkToFit();

    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();
	key = gfx_getKey(false);
	if (delayms)
	{
	    int		currentms = (int)TCOD_sys_elapsed_milli() - startms;
	    float	ratio = currentms / (float)delayms;
	    ratio = BOUND(ratio, 0, 1);

	    glbPopUp->fillRect(0, glbPopUp->height()-1, (int)(glbPopUp->width()*ratio+0.5), 1, ATTR_WAITBAR);
	}
	if (key)
	{
	    // Don't let them abort too soon.
	    if (((int)TCOD_sys_elapsed_milli() - startms) >= delayms)
		break;
	}
    }

    glbPopUpActive = false;
    glbPopUp->erase();

    glbPopUp->resize(50, 30);
    glbPopUp->move(15, 10);

    redrawWorld();

    return key;
}

int
popupText(BUF buf, int delayms = 0)
{
    return popupText(buf.buffer(), delayms);
}

//
// Freezes the game until a key is pressed.
// If key is not direction, false returned and it is eaten.
// else true returned with dx/dy set.  Can be 0,0.
bool
awaitDirection(int &dx, int &dy)
{
    int			key;

    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();

	key = gfx_getKey(false);

	if (gfx_cookDir(key, dx, dy))
	{
	    return true;
	}
	if (key)
	    return false;
    }

    return false;
}

void
doExamine()
{
    int		x = glbDisplay->x() + glbDisplay->width()/2;
    int		y = glbDisplay->y() + glbDisplay->height()/2;
    int		key;
    int		dx, dy;
    POS		p;
    BUF		buf;
    bool	blind;

    glbInvertX = x;
    glbInvertY = y;
    blind = false;
    if (glbMap && glbMap->avatar())
	blind = glbMap->avatar()->hasItem(ITEM_BLIND);
    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();

	key = gfx_getKey(false);

	if (gfx_cookDir(key, dx, dy))
	{
	    // Space we want to select.
	    if (!dx && !dy)
		break;
	    x += dx;
	    y += dy;
	    x = BOUND(x, glbDisplay->x(), glbDisplay->x()+glbDisplay->width()-1);
	    y = BOUND(y, glbDisplay->y(), glbDisplay->y()+glbDisplay->height()-1);

	    glbInvertX = x;
	    glbInvertY = y;
	    p = glbDisplay->lookup(x, y);

	    if (p.tile() != TILE_INVALID && p.isFOV())
	    {
		p.describeSquare(blind);
	    }

	    msg_newturn();
	}

	// Any other key stops the look.
	if (key)
	    break;
    }

    // Check if we left on a mob.
    // Must re look up as displayWorld may have updated the map.
    p = glbDisplay->lookup(x, y);
    if (p.mob())
    {
	popupText(p.mob()->getLongDescription());
    }
    else if (p.item())
    {
	popupText(p.item()->getLongDescription());
    }

    glbInvertX = -1;
    glbInvertY = -1;
}

bool
doThrow(int itemno)
{
    int			dx, dy;

    msg_report("Throw in which direction?  ");
    if (awaitDirection(dx, dy) && (dx || dy))
    {
	msg_report(rand_dirtoname(dx, dy));
	msg_newturn();
    }
    else
    {
	msg_report("Cancelled.  ");
	msg_newturn();
	return false;
    }

    // Do the throw
    glbEngine->queue().append(COMMAND(ACTION_THROW, dx, dy, itemno));

    return true;
}

void
buildAction(ITEM *item, ACTION_NAMES *actions)
{
    glbChooser->clear();

    glbChooser->setTextAttr(ATTR_NORMAL);
    int			choice = 0;
    ACTION_NAMES	*origaction = actions;
    if (!item)
    {
	glbChooser->appendChoice("Nothing to do with nothing.");
	*actions++ = ACTION_NONE;
    }
    else
    {
	glbChooser->appendChoice("[ ] Do nothing");
	if (glbLastItemAction == ACTION_NONE)
	    choice = (int)(actions - origaction);
	*actions++ = ACTION_NONE;

	glbChooser->appendChoice("[X] Examine");
	if (glbLastItemAction == ACTION_EXAMINE)
	    choice = (int)(actions - origaction);
	*actions++ = ACTION_EXAMINE;

	if (item->isRanged())
	{
	    glbChooser->appendChoice("[B] Break");
	    if (glbLastItemAction == ACTION_BREAK)
		choice = (int)(actions - origaction);
	    *actions++ = ACTION_BREAK;
	}
	if (item->isPotion())
	{
	    glbChooser->appendChoice("[Q] Quaff");
	    if (glbLastItemAction == ACTION_QUAFF)
		choice = (int)(actions - origaction);
	    *actions++ = ACTION_QUAFF;
	    glbChooser->appendChoice("[T] Throw");
	    if (glbLastItemAction == ACTION_THROW)
		choice = (int)(actions - origaction);
	    *actions++ = ACTION_THROW;
	}
	if (item->isFood())
	{
	    glbChooser->appendChoice("[E] Eat");
	    if (glbLastItemAction == ACTION_EAT)
		choice = (int)(actions - origaction);
	    *actions++ = ACTION_EAT;
	}
	if (item->isRing())
	{
	    glbChooser->appendChoice("[W] Wear");
	    if (glbLastItemAction == ACTION_WEAR)
		choice = (int)(actions - origaction);
	    *actions++ = ACTION_WEAR;
	}
	if (!item->defn().isflag)
	{
	    glbChooser->appendChoice("[D] Drop");
	    if (glbLastItemAction == ACTION_DROP)
		choice = (int)(actions - origaction);
	    *actions++ = ACTION_DROP;
	}
    }
    glbChooser->setChoice(choice);

    glbChooserActive = true;
}

bool
useItem(MOB *mob, ITEM *item, int itemno)
{
    ACTION_NAMES		actions[NUM_ACTIONS+1];
    bool			done = false;
    int				key;

    buildAction(item, actions);
    glbChooserActive = true;

    // Run the chooser...
    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();
	key = gfx_getKey(false);

	glbChooser->processKey(key);

	if (key)
	{
	    if (key == 'd' || key == 'D')
	    {
		glbEngine->queue().append(COMMAND(ACTION_DROP, itemno));
		done = true;
		break;
	    }
	    else if (key == 'q' || key == 'Q')
	    {
		glbEngine->queue().append(COMMAND(ACTION_QUAFF, itemno));
		done = true;
		break;
	    }
	    else if (key == 'e' || key == 'E')
	    {
		glbEngine->queue().append(COMMAND(ACTION_EAT, itemno));
		done = true;
		break;
	    }
	    else if (key == 'b' || key == 'B')
	    {
		glbEngine->queue().append(COMMAND(ACTION_BREAK, itemno));
		done = true;
		break;
	    }
	    else if (key == 'w' || key == 'W')
	    {
		glbEngine->queue().append(COMMAND(ACTION_WEAR, itemno));
		done = true;
		break;
	    }
	    else if (key == 'x' || key == 'X')
	    {
		popupText(item->getLongDescription());
		done = false;
		break;
	    }
	    else if (key == 't' || key == 'T')
	    {
		glbChooserActive = false;
		doThrow(itemno);
		done = true;
		break;
	    }
	    // User selects this?
	    else if (key == '\n' || key == ' ')
	    {
		ACTION_NAMES	action;

		action = actions[glbChooser->getChoice()];
		glbLastItemAction = action;
		switch (action)
		{
		    case ACTION_DROP:
			glbEngine->queue().append(COMMAND(ACTION_DROP, itemno));
			done = true;
			break;
		    case ACTION_QUAFF:
			glbEngine->queue().append(COMMAND(ACTION_QUAFF, itemno));
			done = true;
			break;
		    case ACTION_BREAK:
			glbEngine->queue().append(COMMAND(ACTION_BREAK, itemno));
			done = true;
			break;
		    case ACTION_WEAR:
			glbEngine->queue().append(COMMAND(ACTION_WEAR, itemno));
			done = true;
			break;
		    case ACTION_EAT:
			glbEngine->queue().append(COMMAND(ACTION_EAT, itemno));
			done = true;
			break;
		    case ACTION_SEARCH:
			glbEngine->queue().append(COMMAND(ACTION_SEARCH, itemno));
			done = true;
			break;
		    case ACTION_MEDITATE:
			glbEngine->queue().append(COMMAND(ACTION_MEDITATE, itemno));
			done = true;
			break;
		    case ACTION_EXAMINE:
			popupText(item->getLongDescription());
			done = false;
			break;
		    case ACTION_THROW:
			glbChooserActive = false;
			doThrow(itemno);
			done = true;
			break;
		}
		break;
	    }
	    else if (key == '\x1b')
	    {
		// Esc on options is to go back to play.
		// Play...
		break;
	    }
	    else
	    {
		// Ignore other options.
	    }
	}
    }
    glbChooserActive = false;

    return done;
}

void
buildInventory(MOB *mob)
{
    glbChooser->clear();

    glbChooser->setTextAttr(ATTR_NORMAL);
    if (!mob)
    {
	glbChooser->appendChoice("The dead own nothing.");
    }
    else
    {
	if (!mob->inventory().entries())
	{
	    glbChooser->appendChoice("nothing");
	}
	else
	{
	    for (int i = 0; i < mob->inventory().entries(); i++)
	    {
		ITEM 		*item = mob->inventory()(i);
		BUF		 name = item->getName();
		if (item->isEquipped())
		{
		    BUF		equip;
		    equip.sprintf("%s (equipped)", name.buffer());
		    name.strcpy(equip);
		}

		glbChooser->appendChoice(name);
	    }
	}
    }
    glbChooser->setChoice(glbLastItemIdx);

    glbChooserActive = true;
}

void
inventoryMenu()
{
    MOB		*avatar = (glbMap ? glbMap->avatar() : 0);
    int		key;
    bool	setlast = false;
    buildInventory(avatar);

    // Run the chooser...
    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();
	key = gfx_getKey(false);

	glbChooser->processKey(key);

	if (key)
	{
	    int		itemno = glbChooser->getChoice();
	    bool	done = false;
	    // User selects this?
	    if (key == '\n' || key == ' ')
	    {
		done = true;

		glbLastItemIdx = glbChooser->getChoice();
		setlast = true;
		if (itemno >= 0 && itemno < avatar->inventory().entries())
		{
		    done = useItem(avatar, avatar->inventory()(itemno), itemno);
		}
		// Finish the inventory menu.
		if (done)
		{
		    break;
		}
		else
		{
		    buildInventory(avatar);
		    setlast = false;
		}
	    }
	    else if (key == '\x1b' || key == 'i')
	    {
		// Esc on options is to go back to play.
		// Play...
		break;
	    }
	    else if (itemno >= 0 && itemno < avatar->inventory().entries())
	    {
		if (key == 'd' || key == 'D')
		{
		    glbEngine->queue().append(COMMAND(ACTION_DROP, itemno));
		    done = true;
		}
		else if (key == 'q' || key == 'Q')
		{
		    glbEngine->queue().append(COMMAND(ACTION_QUAFF, itemno));
		    done = true;
		}
		else if (key == 'e' || key == 'E')
		{
		    glbEngine->queue().append(COMMAND(ACTION_EAT, itemno));
		    done = true;
		}
		else if (key == 'b' || key == 'B')
		{
		    glbEngine->queue().append(COMMAND(ACTION_BREAK, itemno));
		    done = true;
		}
		else if (key == 'w' || key == 'W')
		{
		    glbEngine->queue().append(COMMAND(ACTION_WEAR, itemno));
		    done = true;
		}
		else if (key == 'x' || key == 'X')
		{
		    if (itemno >= 0 && itemno < avatar->inventory().entries())
		    {
			popupText(avatar->inventory()(itemno)->getLongDescription());
		    }
		}
		else if (key == 't' || key == 'T')
		{
		    glbLastItemIdx = glbChooser->getChoice();
		    setlast = true;
		    glbChooserActive = false;
		    doThrow(itemno);
		    done = true;
		    break;
		}
	    }
	    else
	    {
		// Ignore other options.
	    }

	    if (done)
		break;
	}
    }
    if (!setlast)
	glbLastItemIdx = glbChooser->getChoice();
    glbChooserActive = false;
}

void
castSpell(MOB *mob, SPELL_NAMES spell)
{
    int			dx = 0;
    int			dy = 0;

    if (glb_spelldefs[spell].needsdir)
    {
	msg_report("Cast in which direction?  ");
	if (awaitDirection(dx, dy) && (dx || dy))
	{
	    msg_report(rand_dirtoname(dx, dy));
	    msg_newturn();
	}
	else
	{
	    msg_report("Cancelled.  ");
	    msg_newturn();
	    return;
	}
    }
    glbLastSpell = spell;
    glbEngine->queue().append(COMMAND(ACTION_CAST, dx, dy, spell));
}

void
buildZapList(MOB *mob, PTRLIST<SPELL_NAMES> &list)
{
    glbChooser->clear();
    list.clear();
    int				chosen = 0;

    glbChooser->setTextAttr(ATTR_NORMAL);
    if (!mob)
    {
	glbChooser->appendChoice("The dead can cast nothing.");
	list.append(SPELL_NONE);
    }
    else
    {
	SPELL_NAMES		spell;

	FOREACH_SPELL(spell)
	{
	    ITEM_NAMES		itemname;
	    itemname = (ITEM_NAMES) glb_spelldefs[spell].item;
	    if (itemname != ITEM_NONE &&
		mob->hasItem(itemname))
	    {
		BUF		buf;

		buf.sprintf("Cast %s", glb_spelldefs[spell].name);
		glbChooser->appendChoice(buf);
		list.append(spell);
		if (spell == glbLastSpell)
		    chosen = list.entries()-1;
	    }
	}
    }
    if (!glbChooser->entries())
    {
	glbChooser->appendChoice("You know no magic spells.");
	list.append(SPELL_NONE);
    }

    glbChooser->setChoice(chosen);

    glbChooserActive = true;
}

void
zapMenu()
{
    PTRLIST<SPELL_NAMES>	spelllist;
    MOB		*avatar = (glbMap ? glbMap->avatar() : 0);
    int				key;

    buildZapList(avatar, spelllist);

    // Run the chooser...
    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();
	key = gfx_getKey(false);

	glbChooser->processKey(key);

	if (key)
	{
	    int		itemno = glbChooser->getChoice();
	    bool	done = false;
	    // User selects this?
	    if (key == '\n' || key == ' ' || key == 'z' || key == '+')
	    {
		SPELL_NAMES		spell = spelllist(itemno);

		if (spell != SPELL_NONE)
		{
		    glbChooserActive = false;
		    castSpell(avatar, spell);
		}
		break;
	    }
	    else if (key == '\x1b' || key == 'i')
	    {
		// Esc on options is to go back to play.
		// Play...
		break;
	    }
	    else
	    {
		// Ignore other options.
	    }

	    if (done)
		break;
	}
    }
    glbChooserActive = false;
}

void
buildOptionsMenu(OPTION_NAMES d, bool newgame)
{
    OPTION_NAMES	option;
    glbChooser->clear();

    glbChooser->setTextAttr(ATTR_NORMAL);
    FOREACH_OPTION(option)
    {
	if (option == OPTION_PLAY && newgame)
	    glbChooser->appendChoice("New Game");
	else
	    glbChooser->appendChoice(glb_optiondefs[option].name);
    }
    glbChooser->setChoice(d);

    glbChooserActive = true;
}

bool
optionsMenu(bool newgameprompt)
{
    int			key;
    bool		done = false;

    buildOptionsMenu(OPTION_INSTRUCTIONS, newgameprompt);

    // Run the chooser...
    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();
	key = gfx_getKey(false);

	glbChooser->processKey(key);

	if (key)
	{
	    // User selects this?
	    if (key == '\n' || key == ' ')
	    {
		if (glbChooser->getChoice() == OPTION_INSTRUCTIONS)
		{
		    // Instructions.
		    popupText(text_lookup("game", "help"));
		}
		else if (glbChooser->getChoice() == OPTION_PLAY)
		{
		    // Play...
		    if (newgameprompt)
		    {
			reloadLevel(false);
		    }
		    break;
		}
#if 0
		else if (glbChooser->getChoice() == OPTION_VOLUME)
		{
		    int			i;
		    BUF			buf;
		    // Volume
		    glbChooser->clear();

		    for (i = 0; i <= 10; i++)
		    {
			buf.sprintf("%3d%% Volume", (10 - i) * 10);

			glbChooser->appendChoice(buf);
		    }
		    glbChooser->setChoice(10 - glbConfig->myMusicVolume);

		    while (!TCODConsole::isWindowClosed())
		    {
			redrawWorld();
			key = gfx_getKey(false);

			glbChooser->processKey(key);

			setMusicVolume(10 - glbChooser->getChoice());
			if (key)
			{
			    break;
			}
		    }
		    buildOptionsMenu(OPTION_VOLUME);
		}
#endif
		else if (glbChooser->getChoice() == OPTION_BLOOD)
		{
		    glbConfig->myBloodDynamic = !glbConfig->myBloodDynamic;
		    glbHealthFire->setBarGraph(!glbConfig->myBloodDynamic);
		}
		else if (glbChooser->getChoice() == OPTION_FULLSCREEN)
		{
		    glbConfig->myFullScreen = !glbConfig->myFullScreen;
		    // This is intentionally unrolled to work around a
		    // bool/int problem in libtcod
		    if (glbConfig->myFullScreen)
			TCODConsole::setFullscreen(true);
		    else
			TCODConsole::setFullscreen(false);
		}
		else if (glbChooser->getChoice() == OPTION_QUIT)
		{
		    // Quit
		    done = true;
		    break;
		}
	    }
	    else if (key == '\x1b')
	    {
		// Esc on options is to go back to play.
		// Play...
		// But we can't if it is a new game prompt!
		if (!newgameprompt)
		    break;
	    }
	    else
	    {
		// Ignore other options.
	    }
	}
    }
    glbChooserActive = false;

    return done;
}

bool
reloadLevel(bool alreadybuilt)
{
    if (TCODConsole::isWindowClosed())
	return true;

    glbVeryFirstRun = false;

    msg_newturn();
    glbMessagePanel->clear();
    glbMessagePanel->appendText("> ");
    if (!alreadybuilt)
	glbEngine->queue().append(COMMAND(ACTION_RESTART, ATLAS_THRONE));

    startMusic();

    popupText(text_lookup("welcome", "You1"), POPUP_DELAY);

    // Wait for the map to finish.
    popupTextOpen("Building World\nPlease Wait...\n\n");
    if (glbEngine->awaitRebuild())
    {
	popupTextClose();
	popupText(text_lookup("welcome", "Back"), 0);
    }
    else
	popupTextClose();

    // Redrawing the world updates our glbMap so everyone will see it.
    redrawWorld();

    return false;
}

// Let the avatar watch the end...
void
deathCoolDown()
{
    int		coolstartms = TCOD_sys_elapsed_milli();
    int		lastenginems = coolstartms;
    int		curtime;
    bool	done = false;
    double	ratio;

    while (!done)
    {
	curtime = TCOD_sys_elapsed_milli();

	// Queue wait events, one per 100 ms.
	while (lastenginems + 200 < curtime)
	{
	    glbEngine->queue().append(COMMAND(ACTION_WAIT));
	    lastenginems += 200;
	}

	// Set our fade.
	ratio = (curtime - coolstartms) / 5000.;
	ratio = 1.0 - ratio;
	if (ratio < 0)
	{
	    done = true;
	    ratio = 0;
	}

	TCOD_console_set_fade( (u8) (255 * ratio), TCOD_black);

	// Keep updating
	redrawWorld();
    }

    gfx_clearKeyBuf();

    TCOD_console_set_fade(255,  TCOD_black);
}

void
shutdownEverything()
{
    glbConfig->save(SWORD_CFG_DIR "sword.cfg");

    // Save the game.  A no-op if dead.
    glbEngine->queue().append(COMMAND(ACTION_SAVE));
    glbEngine->awaitSave();

    stopMusic(true);
#ifdef USE_AUDIO
    if (glbTunes)
	Mix_FreeMusic(glbTunes);
#endif

    gfx_shutdown();

#ifdef USE_AUDIO
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
#endif

    SDL_Quit();
}

void
verifyQuestStart(QUEST_NAMES quest)
{
    glbChooser->clear();

    glbChooser->setPrequel(text_lookup("quest", glb_questdefs[quest].descr, "confirm"));

    glbChooser->appendChoice("This quest sounds suitable.");
    glbChooser->appendChoice("Let me think on my choice.");
    glbChooser->setChoice(1);
    glbChooserActive = true;

    // Run the chooser...
    while (!TCODConsole::isWindowClosed())
    {
	int	key;
	redrawWorld();
	key = gfx_getKey(false);

	glbChooser->processKey(key);

	if (key)
	{
	    int		itemno = glbChooser->getChoice();
	    bool	done = false;
	    // User selects this?
	    if (key == '\n' || key == ' ' || key == 'z')
	    {
		if (itemno == 0)
		{
		    // Start the quest!
		    glbEngine->queue().append(COMMAND(ACTION_STARTQUEST, quest));
		}
		break;
	    }
	    else if (key == '\x1b' || key == 'i')
	    {
		// Esc on options is to go back to play.
		// Play...
		break;
	    }
	    else
	    {
		// Ignore other options.
	    }

	    if (done)
		break;
	}
    }
    glbChooserActive = false;
}

void
buildShopMenu(MOB_NAMES keeper, PTRLIST<COMMAND> &list)
{
    glbChooser->clear();
    list.clear();
    BUF			buf;
    MOB		*avatar = (glbMap ? glbMap->avatar() : 0);
    bool	valid = false;

    if (!avatar || !avatar->alive())
	return;

    glbChooser->setPrequelAttr((ATTR_NAMES) glb_mobdefs[keeper].attr);
    switch (keeper)
    {
	case MOB_KING:
	{
	    switch (glbWorldState)
	    {
		case WORLDSTATE_CHOOSEQUEST:
		    if (glbLevel == 1)
			glbChooser->setPrequel(text_lookup("shop", "king", "intro"));
		    else
			glbChooser->setPrequel(text_lookup("shop", "king", "choose"));
		    break;
		case WORLDSTATE_ENDQUEST:
		    glbChooser->setPrequel(text_lookup("shop", "king", "done"));
		    break;
		case WORLDSTATE_WON:
		    glbChooser->setPrequel(text_lookup("shop", "king", "won"));
		    break;
	    }
	    break;
	}
	case MOB_ADVISOR_WAR:
	case MOB_ADVISOR_PEACE:
	{
	    const char *name = "war";
	    if (keeper == MOB_ADVISOR_PEACE)
		name = "peace";

	    switch (glbWorldState)
	    {
		case WORLDSTATE_CHOOSEQUEST:
		    if (glbLevel == 1)
			glbChooser->setPrequel(text_lookup("shop", name, "intro"));
		    else
			glbChooser->setPrequel(text_lookup("shop", name, "choose"));
		    break;
		case WORLDSTATE_ENDQUEST:
		    if (glb_questdefs[glbLastQuest].giver == keeper)
			glbChooser->setPrequel(text_lookup("shop", name, "donegood"));
		    else
			glbChooser->setPrequel(text_lookup("shop", name, "donebad"));
		    break;
		case WORLDSTATE_WON:
		    glbChooser->setPrequel(text_lookup("shop", name, "won"));
		    break;
		break;
	    }
	}
    }

    if (glbWorldState == WORLDSTATE_CHOOSEQUEST)
    {
	QUEST_NAMES		quest;
	int			questlevel = glbLevel;
	if (glbLevel > 5)
	{
	    // We are out of quests here so just reuse.
	    // It's a silly quest you are not supposed to get to,
	    // but better that than stuck games!
	    questlevel = 5;
	}
	FOREACH_QUEST(quest)
	{
	    if (glb_questdefs[quest].giver == keeper && glb_questdefs[quest].level == questlevel)
	    {
		BUF		buf;
		buf.sprintf("Quest for %s!", glb_questdefs[quest].name);
		glbChooser->appendChoice(buf);
		list.append(COMMAND(ACTION_STARTQUEST, quest));
	    }
	}
    }

    if (valid)
    {
	glbChooser->appendChoice("Not now, human.");
	list.append(COMMAND(ACTION_NONE));
	glbChooser->setChoice(list.entries()-1);
	glbChooserActive = true;
    }
    else
    {
	glbChooser->appendChoice("Ah, the idle prattle of a human.");
	list.append(COMMAND(ACTION_NONE));
	glbChooser->setChoice(list.entries()-1);
	glbChooserActive = true;
    }
}

void
runShop(MOB_NAMES shopkeeper)
{
    PTRLIST<COMMAND>	shoppinglist;
    int			key;

    buildShopMenu(shopkeeper, shoppinglist);
    
    if (shoppinglist.entries() == 0)
	return;

    // Run the chooser...
    while (!TCODConsole::isWindowClosed())
    {
	redrawWorld();
	key = gfx_getKey(false);

	glbChooser->processKey(key);

	if (key)
	{
	    int		itemno = glbChooser->getChoice();
	    bool	done = false;
	    // User selects this?
	    if (key == '\n' || key == ' ' || key == 'z')
	    {
		COMMAND		cmd = shoppinglist(itemno);
		if (cmd.action() != ACTION_NONE)
		{
		    if (cmd.action() == ACTION_STARTQUEST)
		    {
			return verifyQuestStart((QUEST_NAMES)cmd.dx());
		    }
		    glbEngine->queue().append(cmd);
		}
		break;
	    }
	    else if (key == '\x1b' || key == 'i')
	    {
		// Esc on options is to go back to play.
		// Play...
		break;
	    }
	    else
	    {
		// Ignore other options.
	    }

	    if (done)
		break;
	}
    }
    glbChooserActive = false;
}

void
runMapTable()
{
    glbStrategyViewActive = true;

    popupTextOpen("Rendering Stragetic Map\nPlease Wait...\n\n");
    glbEngine->awaitStrategyUpdate();
    popupTextClose();

    STRATEGY	*strategy = glbEngine->copyStrategy();
    int		week = 0;

    glbStrategyView->setStrategy(strategy);
    glbStrategyView->rewind();

    bool		play = true;
    int			playdir = 1;

    // Run the chooser...
    while (!TCODConsole::isWindowClosed())
    {
	// Update the jacobi with our strategy stuff
	{
	    glbRHSBar->clear();
	    glbRHSBar->setTextAttr(ATTR_WHITE);
	    BUF		buf;
	    CIVSTATE	*today = glbStrategyView->getToday();

	    if (today)
	    {
		glbRHSBar->setTextAttr(ATTR_WHITE);

		buf.sprintf("\nMonth: %d\n\n", glbStrategyView->getYear());
		week++;
		glbRHSBar->appendText(buf);

		glbRHSBar->setTextAttr(ATTR_LAVENDER);
		buf.sprintf("King Viola:\nCities: %5d\nPop:  %7d\nBorn: %7d\nLost: %7d\nArmy: %7d\nStrength: %.1f\n\n",
			glbStrategyView->getCityCount(0),
			(int)today->myDemo[0].pop,
			(int)today->myDemo[0].popgrowth,
			(int)today->myDemo[0].died,
			(int)today->myDemo[0].enlisted,
			today->myDemo[0].attack);
		glbRHSBar->appendText(buf);

		glbRHSBar->setTextAttr(ATTR_CRIMSON);
		buf.sprintf("King Crimson:\nCities: %5d\nPop:  %7d\nBorn: %7d\nLost: %7d\nArmy: %7d\nStrength: %.1f\n\n",
			glbStrategyView->getCityCount(1),
			(int)today->myDemo[1].pop,
			(int)today->myDemo[1].popgrowth,
			(int)today->myDemo[1].died,
			(int)today->myDemo[1].enlisted,
			today->myDemo[1].attack);
		glbRHSBar->appendText(buf);

		glbRHSBar->setTextAttr(ATTR_NORMAL);
		glbRHSBar->appendText("\nDirection Keys to control playback.\n\nEsc to exit.\n");
	    }
	}

	redrawWorld();

	int		key = gfx_getKey(false);
	int		dx, dy;

	if (gfx_cookDir(key, dx, dy))
	{
	    if (dx && dy)
	    {
		// Check for home and end!
		play = false;
		glbStrategyView->rewind();
		if (dy > 0)
		{
		    // End means we go back one!
		    glbStrategyView->playFrame(-1);
		}
	    }
	    else if (dx)
	    {
		play = false;
		glbStrategyView->playFrame(dx);
	    }
	    else if (dy)
	    {
		play = true;
		playdir = -dy;
	    }
	    else
	    {
		// Probably space.
		play = !play;
	    }
	}
	if (key)
	{
	    bool	done = false;

	    if (key == '\n' || key == ' ' || key == 'z')
	    {
		play = !play;
	    }
	    else if (key == '\x1b')
	    {
		// Esc on options is to go back to play.
		// Play...
		break;
	    }
	    else
	    {
		// Ignore other options.
	    }

	    if (done)
		break;
	}

	if (play)
	{
	    glbStrategyView->playFrame(playdir);
	    // See if we hit the end.
	    if (playdir == 1 && (glbStrategyView->getYear() == strategy->getMaxYear()))
	    {
		play = false;
	    }
	    else if (playdir == -1 && (glbStrategyView->getYear() == 0))
	    {
		play = false;
	    }
	}
    }

    // Update our quest status if necessary.
    if (glbWorldState == WORLDSTATE_ENDQUEST)
    {
	int		citycount[2];
	citycount[0] = strategy->cityCount(0, strategy->getToday());
	citycount[1] = strategy->cityCount(1, strategy->getToday());

	if (!citycount[0])
	{
	    // Death of Viola!
	    glbEngine->queue().append(COMMAND(ACTION_WORLDSTATE, WORLDSTATE_LOST));

	}
	else if (!citycount[1])
	{
	    // Death of Crimson!
	    glbEngine->queue().append(COMMAND(ACTION_WORLDSTATE, WORLDSTATE_WON));
	}
	else
	{
	    // Keep on plugging!
	    glbEngine->queue().append(COMMAND(ACTION_WORLDSTATE, WORLDSTATE_CHOOSEQUEST));
	}
    }

    glbRHSBar->clear();
    glbStrategyViewActive = false;
}


void
computeStats()
{
    int		winpattern = 0;
    ofstream	os("winstats.txt");

    for (winpattern = 0; winpattern < 16; winpattern++)
    {
	STRATEGY 	*strategy = new STRATEGY(ATLAS_STRATEGY);
	strategy->incRef();
	bool		 live = true;

	for (int i = 0; i < 24; i++)
	    strategy->integrate();

	for (int j = 0; j < 5; j++)
	{
	    QUEST_NAMES		quest;
	    if (winpattern & (1 << j))
	    {
		os << "X";
		quest = QUEST_FIRE;
	    }
	    else
	    {
		os << "O";
		quest = QUEST_GRAIN;
	    }
	    strategy->grantQuestBonus(0, quest);
	    for (int i = 1; i < 36; i++)
	    {
		// Update strategy...
		live = strategy->integrate();
		if (!live)
		    break;
	    }
	    if (live)
	    {
		strategy->grantQuestBonus(1, quest);
		live = strategy->integrate();
	    }
	    if (!live)
		break;
	}
	CIVSTATE		*today = strategy->getToday();
	if (!live)
	{
	    os << " - finished - ";
	    if (strategy->cityCount(0, today))
		os << "Viola";
	    else
		os << "Crimson";

	}
	os << endl;

	os << "Year " << strategy->getMaxYear() << endl;
	os << "Pop:  " << today->myDemo[0].pop  + today->myDemo[1].pop << endl;
	os << "Kill: " << today->myDemo[0].died  + today->myDemo[1].died << endl;
	float	totalpop = today->myDemo[0].pop + today->myDemo[1].pop;
	float	totalgrowth = MAX(today->myDemo[0].popgrowth, today->myDemo[1].popgrowth);
    // Time to get to 2.2 M again.
    // 12 is for month -> year
    // 11 is for cities now that war is over.
    float	 rebuildtime = (2.2*1000000 - totalpop) / (totalgrowth * 12 * 11);
    // Normalize rebuild time so higher pops rebuild faster.
    rebuildtime /= (totalpop / (1000*1000));
    os << "Rebuild: " << rebuildtime << endl;

	for (int i = 0; i < 2; i++)
	{
	    if (i)
		os << "King Crimson" << endl;
	    else
		os << "King Viola" << endl;
	    os << "Pop:  " << today->myDemo[i].pop << endl;
	    os << "Born: " << today->myDemo[i].popgrowth << endl;
	    os << "Army: " << today->myDemo[i].enlisted << endl;
	    os << "Kill: " << today->myDemo[i].died << endl;
	}
	os << endl;

	strategy->decRef();
    }
}

void
reportVictory()
{
    STRATEGY	*strategy = glbEngine->copyStrategy();

    CIVSTATE	*today = strategy->getToday();

    float	 totalpop = today->myDemo[0].pop + today->myDemo[1].pop;
    // We take the max as there will be tech transfer.
    float	 totalgrowth = MAX(today->myDemo[0].popgrowth, today->myDemo[1].popgrowth);
    float	 totaldied = today->myDemo[0].died + today->myDemo[1].died;
    // Time to get to 2.2 M again.
    // 12 is for month -> year
    // 11 is for cities now that war is over.
    float	 rebuildtime = (2.2*1000000 - totalpop) / (totalgrowth * 12 * 11);

    // Normalize rebuild time so higher pops rebuild faster.
    rebuildtime /= (totalpop / (1000*1000));

    // According to the source of all wisdome, Wikipedia, some towns
    // took over a hundred years to recover from the 30 years war.
    // So a 200 hundred year rebuild time is not unbelievable.
    BUF		 summary;

    summary.sprintf("Surviving: %7d\n"
		    "Killed:    %7d\n"
		    "Rebuild:   %.1f years\n"
		    "\n",
		    (int) totalpop, (int) totaldied,
		    rebuildtime);

    // A simple cut off for the text.
    // <50 years: Merchant class can take hold leading to lasting
    // peace.
    // <100 years: Grim dictatorship crushes rebels ruthlessly
    // >100 years: Anarchy.
    //
    // Note this text does not depend on the victor.  We really
    // don't care what king won 200 years ago.
    if (rebuildtime < 50)
	summary.strcat(text_lookup("theend", "fast"));
    else if (rebuildtime < 100)
	summary.strcat(text_lookup("theend", "med"));
    else
	summary.strcat(text_lookup("theend", "slow"));

    popupText(summary, POPUP_DELAY);


    strategy->decRef();
}

#ifdef WIN32
int WINAPI
WinMain(HINSTANCE hINstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCMdSHow)
#elif defined(__APPLE__)
extern C_LINKAGE int SDL_main(int argc, char *argv[])
#else
int 
main(int argc, char **argv)
#endif
{
    bool		done = false;

    // Clamp frame rate.
    TCOD_sys_set_fps(60);

    glbConfig = new CONFIG();
    glbConfig->load(SWORD_CFG_DIR "sword.cfg");

    // Dear Microsoft,
    // The following code in optimized is both a waste and seems designed
    // to ensure that if we call a bool where the callee pretends it is
    // an int we'll end up with garbage data.
    //
    // 0040BF4C  movzx       eax,byte ptr [eax+10h] 

    // TCODConsole::initRoot(SCR_WIDTH, SCR_HEIGHT, "Sword In Hand", myFullScreen);
    // 0040BF50  xor         ebp,ebp 
    // 0040BF52  cmp         eax,ebp 
    // 0040BF54  setne       cl   
    // 0040BF57  mov         dword ptr [esp+28h],eax 
    // 0040BF5B  push        ecx  

    // My work around is to constantify the fullscreen and hope that
    // the compiler doesn't catch on.
    if (glbConfig->myFullScreen)
	TCODConsole::initRoot(SCR_WIDTH, SCR_HEIGHT, "Sword In Hand", true);
    else
	TCODConsole::initRoot(SCR_WIDTH, SCR_HEIGHT, "Sword In Hand", false);

    // TCOD doesn't do audio.
#ifdef USE_AUDIO
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (Mix_OpenAudio(22050, AUDIO_S16, 2, 4096))
    {
	printf("Failed to open audio!\n");
	exit(1);
    }
#endif

    SDL_InitSubSystem(SDL_INIT_JOYSTICK);

    setMusicVolume(glbConfig->musicVolume());

    rand_setseed((long) time(0));

    gfx_init();
    MAP::init();

    text_init();
    spd_init();

    glbDisplay = new DISPLAY(0, 0, 80, 40);

    glbStrategyView = new STRAGETICVIEW(17, 2, 45, 45);
    glbStrategyViewActive = false;

    glbMessagePanel = new PANEL(40, 10);
    msg_registerpanel(glbMessagePanel);
    glbMessagePanel->move(20, 40);

    glbPopUp = new PANEL(50, 30);
    glbPopUp->move(15, 10);
    glbPopUp->setBorder(true, ' ', ATTR_BORDER);
    // And so the mispelling is propagated...
    glbPopUp->setRigthMargin(1);
    glbPopUp->setIndent(1);

    glbJacob = new PANEL(15, 6);
    glbJacob->move(65, 1);
    glbJacob->setTransparent(true);

    glbRHSBar = new PANEL(15, 40);
    glbRHSBar->move(65, 4);
    glbRHSBar->setTransparent(true);

    glbWeaponInfo = new PANEL(20, 20);
    glbWeaponInfo->move(1, 2);
    glbWeaponInfo->setTransparent(true);

    glbVictoryInfo = new PANEL(26, 2);
    glbVictoryInfo->move(28, 7);
    glbVictoryInfo->setIndent(2);
    glbVictoryInfo->setBorder(true, ' ', ATTR_VICTORYBORDER);

    glbChooser = new CHOOSER();
    glbChooser->move(40, 25, CHOOSER::JUSTCENTER, CHOOSER::JUSTCENTER);
    glbChooser->setBorder(true, ' ', ATTR_BORDER);
    glbChooser->setIndent(1);
    glbChooser->setRigthMargin(1);

    glbLevelBuildStatus = new CHOOSER();
    glbLevelBuildStatus->move(40, 25, CHOOSER::JUSTCENTER, CHOOSER::JUSTCENTER);
    glbLevelBuildStatus->setBorder(true, ' ', ATTR_BORDER);
    glbLevelBuildStatus->setIndent(1);
    glbLevelBuildStatus->setRigthMargin(1);
    glbLevelBuildStatus->setAttr(ATTR_NORMAL, ATTR_HILITE, ATTR_HILITE);

    glbEngine = new ENGINE(glbDisplay);

    glbHealthFire = new FIREFLY(0, 20, 49, FIRE_BLOOD, SWORD_CFG_DIR "gfx/sword.txt", false);
    glbHealthFire->setBarGraph(!glbConfig->myBloodDynamic);

    // Run our first level immediately so it can be built or 
    // loaded from disk while people wait.
    glbEngine->queue().append(COMMAND(ACTION_RESTART, 1));

    // I've decided against starting with the options menu.  People
    // can hit ? and this gets you into the game faster.
#if 0
    done = optionsMenu(false);
    if (done)
    {
	shutdownEverything();
	return 0;
    }
#endif

    done = reloadLevel(true);
    if (done)
    {
	shutdownEverything();
	return 0;
    }

    do
    {
	int		key;
	int		dx, dy;

	redrawWorld();

	while (1)
	{
	    BUF		pop;

	    pop = glbEngine->getNextPopup();
	    if (!pop.isstring())
		break;

	    popupText(pop, POPUP_DELAY);
	}

	while (1)
	{
	    MOB_NAMES	shopper;

	    shopper = glbEngine->getNextShopper();
	    if (shopper == MOB_NONE)
		break;

	    if (shopper == MOB_MAPSHOP)
	    {
		runMapTable();
	    }
	    else
		runShop(shopper);
	}


	if (glbMap && glbMap->avatar() &&
	    !glbMap->avatar()->alive())
	{
	    // You have died...
	    deathCoolDown();

	    if (glbWorldState == WORLDSTATE_QUEST)
		popupText(text_lookup("game", "lose"), POPUP_DELAY);
	    else
		popupText(text_lookup("game", "betrayal"), POPUP_DELAY);

	    stopMusic();

	    done = optionsMenu(true);
	    if (done)
	    {
		shutdownEverything();
		return 0;
	    }
	}

	key = gfx_getKey(false);

	if (gfx_cookDir(key, dx, dy))
	{
	    // Direction.
	    glbEngine->queue().append(COMMAND(ACTION_BUMP, dx, dy));
	}

	switch (key)
	{
	    case 'a':
	    case 's':
	    case 'd':
	    case 'f':
	    case 'g':
	    {
		MAGICMOVE_NAMES		move = getMoveFromKey(key);
		BUF			buf;

		if (move == MAGICMOVE_NONE)
		    break;

		buf.sprintf("%s in which direction?  ",
			    glb_magicmovedefs[move].name);

		msg_report(buf);
		if (awaitDirection(dx, dy) && (dx || dy))
		{
		    msg_report(rand_dirtoname(dx, dy));
		    msg_newturn();
		    glbEngine->queue().append(COMMAND(ACTION_MAGICMOVE, dx, dy, move));
		}
		else
		{
		    msg_report("Cancelled.  ");
		    msg_newturn();
		}
		break;
	    }
	    case 'Q':
		done = true;
		break;

	    case 'R':
		done = reloadLevel(false);
		break;

	    case 'S':
		glbEngine->queue().append(COMMAND(ACTION_SEARCH));
		break;

	    case 'i':
		inventoryMenu();
		break;

	    case 'A':
		popupText(text_lookup("game", "about"));
		break;

	    case 'W':
	    {
		popupText(text_lookup("welcome", "You1"));
		break;
	    }

	    case 'P':
		glbConfig->myFullScreen = !glbConfig->myFullScreen;
		// This is intentionally unrolled to work around a
		// bool/int problem in libtcod
		if (glbConfig->myFullScreen)
		    TCODConsole::setFullscreen(true);
		else
		    TCODConsole::setFullscreen(false);
		break;
	
	    case GFX_KEYF1:
	    case '?':
		popupText(text_lookup("game", "help"));
		break;

	    case 'v':
		if (glbMap && glbMap->avatar())
		{
		    if (glbWorldState == WORLDSTATE_WON ||
			glbWorldState == WORLDSTATE_LOST)
		    {
			reportVictory();

			done = optionsMenu(true);
		    }
		    else
		    {
			msg_report("You haven't achieved victory conditions.  ");
		    }
		}
		break;

	    case 'x':
		doExamine();
		break;

	    case '\x1b':
		// If meditating, abort.
		if (glbMap && glbMap->avatar() && 
		    glbMap->avatar()->alive() && glbMap->avatar()->isMeditating())
		{
		    glbEngine->queue().append(COMMAND(ACTION_MEDITATE));
		    break;
		}
		// FALL THROUGH
	    case 'O':
		done = optionsMenu(false);
		break;

#if 0
	    case 'z':
		glbEngine->queue().append(COMMAND(ACTION_ENDQUEST, QUEST_GRAIN));
		break;
	    case 'c':
		glbEngine->queue().append(COMMAND(ACTION_ENDQUEST, QUEST_FIRE));
		break;

	    case 'q':
		computeStats();
		break;

#endif
	}
    } while (!done && !TCODConsole::isWindowClosed());

    shutdownEverything();

    return 0;
}
