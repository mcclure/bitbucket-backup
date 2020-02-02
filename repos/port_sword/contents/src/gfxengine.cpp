/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        gfxengine.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 * 	This is a very light wrapper around libtcod.
 */

#include <libtcod.hpp>
#include "gfxengine.h"

TCODNoise		*glbPulseNoise = 0;
float			 glbPulseVal = 1.0f;

#ifdef __APPLE__
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

SDL_Joystick		*glbJoyStick = 0;
int			 glbJoyStickNum = 0;
int			 glbJoyDelay = 100, glbJoyRepeat = 60, glbButtonRepeat = 300;

int
cookKey(TCOD_key_t key)
{
    switch (key.vk)
    {
	case TCODK_CHAR:
	    return key.c;

	case TCODK_ENTER:
	case TCODK_KPENTER:
	    return '\n';

	case TCODK_BACKSPACE:
	    return '\b';
	
	case TCODK_ESCAPE:
	    return '\x1b';

	case TCODK_TAB:
	    return '\t';

	case TCODK_SPACE:
	    return ' ';

	case TCODK_UP:
	    return GFX_KEYUP;
	case TCODK_DOWN:
	    return GFX_KEYDOWN;
	case TCODK_LEFT:
	    return GFX_KEYLEFT;
	case TCODK_RIGHT:
	    return GFX_KEYRIGHT;

	case TCODK_PAGEUP:
	    return GFX_KEYPAGEUP;
	case TCODK_PAGEDOWN:
	    return GFX_KEYPAGEDOWN;
	case TCODK_HOME:
	    return GFX_KEYHOME;
	case TCODK_END:
	    return GFX_KEYEND;

	case TCODK_F1:
	    return GFX_KEYF1;
	case TCODK_F2:
	    return GFX_KEYF2;
	case TCODK_F3:
	    return GFX_KEYF3;
	case TCODK_F4:
	    return GFX_KEYF4;
	case TCODK_F5:
	    return GFX_KEYF5;
	case TCODK_F6:
	    return GFX_KEYF6;
	case TCODK_F7:
	    return GFX_KEYF7;
	case TCODK_F8:
	    return GFX_KEYF8;
	case TCODK_F9:
	    return GFX_KEYF9;
	case TCODK_F10:
	    return GFX_KEYF10;
	case TCODK_F11:
	    return GFX_KEYF11;
	case TCODK_F12:
	    return GFX_KEYF12;

	case TCODK_0:
	case TCODK_1:
	case TCODK_2:
	case TCODK_3:
	case TCODK_4:
	case TCODK_5:
	case TCODK_6:
	case TCODK_7:
	case TCODK_8:
	case TCODK_9:
	    return '0' + key.vk - TCODK_0;

	case TCODK_KP0:
	case TCODK_KP1:
	case TCODK_KP2:
	case TCODK_KP3:
	case TCODK_KP4:
	case TCODK_KP5:
	case TCODK_KP6:
	case TCODK_KP7:
	case TCODK_KP8:
	case TCODK_KP9:
	    return '0' + key.vk - TCODK_KP0;

	case TCODK_KPADD:
	    return '+';
	case TCODK_KPSUB:
	    return '-';
	case TCODK_KPDIV:
	    return '/';
	case TCODK_KPMUL:
	    return '*';

	// To match up the keypad, hopefully.
	case TCODK_INSERT:
	    return '0';

	case TCODK_KPDEC:	// I think keypad del?
	    return '.';
    }

    return 0;
}

bool
gfx_cookDir(int &key, int &dx, int &dy, GFX_Cookdir cookdir)
{
    // Check diagonals only if both x & y.
    if ((cookdir & GFX_COOKDIR_X) && (cookdir & GFX_COOKDIR_Y))
    {
	switch (key)
	{
	    case GFX_KEYHOME:
	    case '7':
	    case 'y':
		dx = -1;	dy = -1;
		key = 0;
		return true;
	    case GFX_KEYPAGEUP:
	    case '9':
	    case 'u':
		dx = 1;	dy = -1;
		key = 0;
		return true;
	    case GFX_KEYEND:
	    case '1':
	    case 'b':
		dx = -1;	dy = 1;
		key = 0;
		return true;
	    case GFX_KEYPAGEDOWN:
	    case '3':
	    case 'n':
		dx = 1;	dy = 1;
		key = 0;
		return true;
	}
    }

    // Check left/right
    if (cookdir & GFX_COOKDIR_X)
    {
	switch (key)
	{
	    case GFX_KEYLEFT:
	    case '4':
	    case 'h':
		dx = -1;	dy = 0;
		key = 0;
		return true;
	    case GFX_KEYRIGHT:
	    case '6':
	    case 'l':
		dx = 1;	dy = 0;
		key = 0;
		return true;
	}
    }

    // Check up/down
    if (cookdir & GFX_COOKDIR_Y)
    {
	switch (key)
	{
	    case GFX_KEYDOWN:
	    case '2':
	    case 'j':
		dx = 0;	dy = 1;
		key = 0;
		return true;
	    case GFX_KEYUP:
	    case '8':
	    case 'k':
		dx = 0;	dy = -1;
		key = 0;
		return true;
	}
    }

    // Check stationary
    if (cookdir & GFX_COOKDIR_STATIONARY)
    {
	switch (key)
	{
	    case '5':
	    case '.':
	    case ' ':
		dx = 0;	dy = 0;
		key = 0;
		return true;
	}
    }

    return false;
}

int 
cookJoyKeys()
{
    if (!glbJoyStick)
	return 0;

    SDL_JoystickUpdate();

    int		xstate, ystate;
    int		key = 0;
    int		nbutton = SDL_JoystickNumButtons(glbJoyStick);
    int		button;

    if (nbutton > 16)
	nbutton = 16;

    static int	olddir = 4;
    int		dir = 0;
    static int	dirstartms;
    static bool dirrepeating;

    static bool buttonrepeating[16];
    static bool buttonold[16];
    bool 	buttonstate[16];
    static int  buttonstartms[16];

    const int	keymapping[9] =
    { GFX_KEYHOME, GFX_KEYUP, GFX_KEYPAGEUP,
      GFX_KEYLEFT, 0, GFX_KEYRIGHT,
      GFX_KEYEND, GFX_KEYDOWN, GFX_KEYPAGEDOWN,
    };

    const int buttonmapping[16] =
    {
	'f', ' ', '+', '-',
	'/', '*', '/', '*',
	'v', 'O', 0, 0,
	0, 0, 0, 0
    };

    // Transform the axis direction into a 9-way direction, y high trit.

    xstate = SDL_JoystickGetAxis(glbJoyStick, 0);
    ystate = SDL_JoystickGetAxis(glbJoyStick, 1);

    if (xstate > 16384)
	dir += 2;
    else if (xstate > -16384)
	dir++;

    if (ystate > 16384)
	dir += 6;
    else if (ystate > -16384)
	dir += 3;

    // Check for key presses.
    if (dir != olddir)
    {
	olddir = dir;
	key = keymapping[dir];
	dirstartms = (int)TCOD_sys_elapsed_milli();
	dirrepeating = false;
    }
    else
    {
	int	delay = dirrepeating ? glbJoyRepeat : glbJoyDelay;

	if (((int) TCOD_sys_elapsed_milli() - dirstartms) > delay)
	{
	    key = keymapping[dir];
	    dirstartms = (int)TCOD_sys_elapsed_milli();
	    dirrepeating = true;
	}
    }

    if (key)
	return key;

    for (button = 0; button < nbutton; button++)
    {
	buttonstate[button] = SDL_JoystickGetButton(glbJoyStick, button) ? true : false;
	if (buttonstate[button] != buttonold[button])
	{
	    buttonold[button] = buttonstate[button];
	    if (buttonstate[button])
	    {
		buttonstartms[button] = (int) TCOD_sys_elapsed_milli();
		buttonrepeating[button] = false;
		key = buttonmapping[button];
	    }
	}
	else if (buttonstate[button])
	{
	    int		delay = glbButtonRepeat;

	    if (((int) TCOD_sys_elapsed_milli() - buttonstartms[button]) > delay)
	    {
		key = buttonmapping[dir];
		buttonstartms[button] = (int)TCOD_sys_elapsed_milli();
		buttonrepeating[button] = true;
	    }
	}

	if (key)
	    return key;
    }

    return key;
}

void
gfx_clearKeyBuf()
{
    while (gfx_getKey(false));
}

int
gfx_getKey(bool block)
{
    int		key = 0;
    TCOD_key_t  tkey;

    do
    {
	TCODConsole::flush();
	tkey = TCODConsole::checkForKeypress(TCOD_KEY_PRESSED);
	key = cookKey(tkey);
	if (!key)
	{
	    key = cookJoyKeys();
	}
    } while (block && !key);

    return key;
}

void
gfx_update()
{
    TCODConsole::flush();
}

void
gfx_init()
{
    glbPulseNoise = new TCODNoise(1, 0.5, 2.0);

    glbJoyStick = 0;

    if (SDL_NumJoysticks() > glbJoyStickNum)
    {
	glbJoyStick = SDL_JoystickOpen(glbJoyStickNum);

	if (glbJoyStick)
	{
	    // printf("Opened joystick %s\n", SDL_JoystickName(glbJoyStickNum));
	}
	else
	{
	    printf("Failed to open joystick %d!\n", glbJoyStickNum);
	}
    }
}

void
gfx_shutdown()
{
    if (SDL_JoystickOpened(glbJoyStickNum))
	SDL_JoystickClose(glbJoyStick);
}

void
gfx_updatepulsetime()
{
    int 	timems = TCOD_sys_elapsed_milli();
    float	t;

    // Wrap every minute to keep noise in happy space.
    timems %= 60 * 1000;

    t = timems / 1000.0F;

    glbPulseVal = glbPulseNoise->getTurbulenceWavelet(&t, 6) * 2 + 1.0f;
}


void 
gfx_printchar(int x, int y, u8 c, ATTR_NAMES attr)
{
    if (x < 0 || y < 0) return;
    if (x >= SCR_WIDTH || y >= SCR_HEIGHT) return;

    TCODColor		fg(glb_attrdefs[attr].fg_r,
			   glb_attrdefs[attr].fg_g,
			   glb_attrdefs[attr].fg_b);

    if (glb_attrdefs[attr].pulse)
	fg = fg * glbPulseVal;

    TCODConsole::root->setBack(x, y, TCODColor(glb_attrdefs[attr].bg_r,
					       glb_attrdefs[attr].bg_g,
					       glb_attrdefs[attr].bg_b));
    TCODConsole::root->setFore(x, y, fg);
    TCODConsole::root->setChar(x, y, c);
}

void 
gfx_printattr(int x, int y, ATTR_NAMES attr)
{
    if (x < 0 || y < 0) return;
    if (x >= SCR_WIDTH || y >= SCR_HEIGHT) return;

    TCODColor		fg(glb_attrdefs[attr].fg_r,
			   glb_attrdefs[attr].fg_g,
			   glb_attrdefs[attr].fg_b);

    if (glb_attrdefs[attr].pulse)
	fg = fg * glbPulseVal;

    TCODConsole::root->setBack(x, y, TCODColor(glb_attrdefs[attr].bg_r,
					       glb_attrdefs[attr].bg_g,
					       glb_attrdefs[attr].bg_b));
    TCODConsole::root->setFore(x, y, fg);
}

void 
gfx_printattrback(int x, int y, ATTR_NAMES attr)
{
    if (x < 0 || y < 0) return;
    if (x >= SCR_WIDTH || y >= SCR_HEIGHT) return;

    TCODConsole::root->setBack(x, y, TCODColor(glb_attrdefs[attr].bg_r,
					       glb_attrdefs[attr].bg_g,
					       glb_attrdefs[attr].bg_b));
}

void 
gfx_printattrfore(int x, int y, ATTR_NAMES attr)
{
    if (x < 0 || y < 0) return;
    if (x >= SCR_WIDTH || y >= SCR_HEIGHT) return;

    TCODColor		fg(glb_attrdefs[attr].fg_r,
			   glb_attrdefs[attr].fg_g,
			   glb_attrdefs[attr].fg_b);

    if (glb_attrdefs[attr].pulse)
	fg = fg * glbPulseVal;

    TCODConsole::root->setFore(x, y, fg);
}

void 
gfx_printchar(int x, int y, u8 c, u8 r, u8 g, u8 b)
{
    if (x < 0 || y < 0) return;
    if (x >= SCR_WIDTH || y >= SCR_HEIGHT) return;

    TCODConsole::root->setBack(x, y, TCODColor(0, 0, 0));
    TCODConsole::root->setFore(x, y, TCODColor(r, g, b));
    TCODConsole::root->setChar(x, y, c);
}


void 
gfx_printchar(int x, int y, u8 c, u8 r, u8 g, u8 b, u8 br, u8 bg, u8 bb)
{
    if (x < 0 || y < 0) return;
    if (x >= SCR_WIDTH || y >= SCR_HEIGHT) return;

    TCODConsole::root->setBack(x, y, TCODColor(br, bg, bb));
    TCODConsole::root->setFore(x, y, TCODColor(r, g, b));
    TCODConsole::root->setChar(x, y, c);
}

void 
gfx_printchar(int x, int y, u8 c)
{
    if (x < 0 || y < 0) return;
    if (x >= SCR_WIDTH || y >= SCR_HEIGHT) return;

    TCODConsole::root->setChar(x, y, c);
}

void 
gfx_fadefromwhite(int x, int y, float fade)
{
    if (x < 0 || y < 0) return;
    if (x >= SCR_WIDTH || y >= SCR_HEIGHT) return;

    TCODColor		fg = TCODConsole::root->getFore(x, y);
    int			grey;

    grey = (int)fg.r + fg.g + fg.b;
    grey /= 3;
    if (fade < 0.5)
    {
	fg.r = 255 * (1-fade*2) + grey * (fade*2);
	fg.g = 0 * (1-fade*2) + grey * (fade*2);
	fg.b = 0 * (1-fade*2) + grey * (fade*2);
    }
    else
    {
	fg.r = grey * (1-(fade-0.5)*2) + fg.r * ((fade-0.5)*2);
	fg.g = grey * (1-(fade-0.5)*2) + fg.g * ((fade-0.5)*2);
	fg.b = grey * (1-(fade-0.5)*2) + fg.b * ((fade-0.5)*2);
    }

    TCODColor		bg = TCODConsole::root->getBack(x, y);

    grey = (int)bg.r + bg.g + bg.b;
    grey /= 3;
    if (fade < 0.5)
    {
	bg.r = 255 * (1-fade*2) + grey * (fade*2);
	bg.g = 0 * (1-fade*2) + grey * (fade*2);
	bg.b = 0 * (1-fade*2) + grey * (fade*2);
    }
    else
    {
	bg.r = grey * (1-(fade-0.5)*2) + bg.r * ((fade-0.5)*2);
	bg.g = grey * (1-(fade-0.5)*2) + bg.g * ((fade-0.5)*2);
	bg.b = grey * (1-(fade-0.5)*2) + bg.b * ((fade-0.5)*2);
    }
    TCODConsole::root->setBack(x, y, bg);
    TCODConsole::root->setFore(x, y, fg);
}

void 
gfx_getString(int x, int y, ATTR_NAMES attr, char *buf, int maxlen)
{
    strcpy(buf, "Not Implemented!");
}
