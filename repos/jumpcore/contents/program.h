/*
 *  program.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 4/27/08.
 *
 */

#ifndef _PROGRAM_H
#define _PROGRAM_H

#include "kludge.h"
#include "chipmunk.h"
#include "controls.h"
#include <math.h>
#include <list>
#include <fstream>

#define NAME_OF_THIS_PROGRAM "Jumpcore"

// The following handful of lines are the only ones you can't erase. Feel free to redefine "FPS".

#ifdef TARGET_DESKTOP // This is terrible
#define FPS 80.0
#else
#define FPS 60.0
#endif

#define TPF ((int)(1000/FPS))
extern int ticks;
extern int sinceLastFrame;
extern bool gotJoystick;
extern bool paused;
cpVect screenToGL(int screenx, int screeny, double desiredZ);

extern cpSpace *uiSpace;
inline cpSpace *workingUiSpace() { return uiSpace; }
bool interface_attempt_click(cpVect screen_coord, int button);

void recreate_surface(bool in_a_window);

// CALLBACKS

void BackOut();
void AboutToQuit();
void Quit(int code = 0);
void BombBox(string why = string());
void FileBombBox(string filename = string());

void program_init(bool reinit = false);
void program_update(void);

#ifdef TARGET_MOBILE
void program_sleep();
void program_wake();
#endif

enum touch_type {
    touch_down,
    touch_move,
    touch_up,
    touch_cancel,
};

#if defined(TARGET_ANDROID) || defined(TARGET_WEBOS)
typedef unsigned int touch_id;
#else
typedef void* touch_id;
#endif

// Add to this structure if there's additional stuff to track in a touch.
struct touch_rec {
    touch_id tid; // Always 0 on desktop
	int button; // Always 0 on mobile. TODO: Should be a mask?
    cpVect at;
	enum { not_special = 0, interface_special } special;
	
	static touch_rec c(touch_id _tid, cpVect _at, int _button = 0) {
		touch_rec r = {_tid, _button, _at};
		return r;
	}
};

#ifdef TARGET_SDL

void program_eventkey(SDL_Event &event);
void program_eventjoy(SDL_Event &event);
void program_eventmouse(SDL_Event &event);
void program_eventrecontroller(SDL_Event &event);

#else // not TARGET_SDL

uint64_t SDL_GetTicks();

#endif

#ifdef TARGET_MOBILE

void program_eventtouch(const list<touch_rec> &touches, touch_type kind);
void program_eventaccel(const float &x, const float &y, const float &z);
#endif

void display(void);
void display_init(bool reinit);
void drawButton(void *ptr, void *data);
void audio_init();
void audio_update();

// Interface / ControlBase

extern vector<ContainerBase *> columns;
void program_interface();
extern bool wantClearUi;
enum WantNewScreen { // TODO: DELETE ?!
	WNothing = 0,
	WMainMenu
};
extern WantNewScreen want; 
extern bool &optWindow;
inline int iabs(int i) { return i < 0 ? -i : i; } // Yeah, I dunno.
#define ZERO(x) memset((x), 0, sizeof(x))

float textWidth(string str);
float textHeight(string str);

extern string displaying;
extern int displaying_life;

void program_reinit(void);

// TODO Support like, a a=b key format?
template<typename T> bool cheatfile_load(T &result, const char *name) {
	ifstream file (name);
	bool success = false;
	if (file.is_open()) {
		file >> result;
		success = file.good();
		file.close();
	}
	return success;
}

// manages "animations"
enum { // automata states
    ASInit = 0
};

// FIXME: Make an ent
class bannerstream : public ostringstream {
public:
    int life;
    bannerstream(int _life) : ostringstream() { life = _life; }
    ~bannerstream();
};

// Only use inside brackets!
#define BANNER(z) bannerstream banner(z); banner

extern bool optDebug;

// End Jumpcore basic stuff-- add your own program state below

#define C_WALL 0x1
#define C_CLUTTER 0x2

#endif