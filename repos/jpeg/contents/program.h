/*
 *  program.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 4/27/08.
 *
 */

#include "kludge.h"
#include "controls.h"
#include <math.h>
#include <list>

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
extern double aspect;
extern bool gotJoystick;
extern bool paused;
extern int surfacew, surfaceh;
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

#ifdef TARGET_SDL

void program_eventkey(SDL_Event &event);
void program_eventjoy(SDL_Event &event);
void program_eventmouse(SDL_Event &event);

#else // not TARGET_SDL

uint64_t SDL_GetTicks();

#endif

#ifdef TARGET_MOBILE

#if defined(TARGET_ANDROID) || defined(TARGET_WEBOS)
typedef unsigned int touch_id;
#else
typedef void* touch_id;
#endif

struct touch_rec {
    touch_id tid;
    cpVect at;
    touch_rec(touch_id _tid, cpVect _at) : tid(_tid), at(_at) {}
};

void program_eventtouch(const list<touch_rec> &touches, touch_type kind);
void program_eventaccel(const float &x, const float &y, const float &z);
#endif

void display(void);
void display_init(bool reinit);
void drawButton(void *ptr, void *data);
void audio_init();
void audio_callback(void *userdata, uint8_t *stream, int len);

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

void goOrtho();
void orthoText();

struct spaceinfo {
	double depth;
	cpSpace *space;
	cpBody *staticBody;

	spaceinfo() {
		depth = 0;
		space = NULL;
		staticBody = NULL;
	}
};

extern vector<spaceinfo> spaces;

inline bool okayToDraw(spaceinfo *) { return true; }

float textWidth(string str);
float textHeight(string str);

extern string displaying;
extern int displaying_life;

struct automaton;
typedef list<automaton *>::iterator auto_iter;
extern list<automaton *> automata;

void program_reinit(void);

// Add to this structure if there's some kind of state you want to track alongside each touch/click
struct dragtouch {
    enum { not_special = 0, interface_special } special; // 
	int button; // Last clicked mouse button. Note: Meaningless on mobile
    dragtouch() : special(not_special), button(0) {}
};

// manages "animations"
struct automaton {
    int born;  // ticks born at
    int frame; // ticks within state
    int state; // which state are you at?
    int rollover; // what frame does state roll at?
    auto_iter anchor;
    automaton() : born(ticks), frame(0), state(0), rollover(1) {}
    inline int age() { return ticks-born; }
    virtual void tick();
    virtual void die();
    void push() {
        automata.push_back(this);
        anchor = --automata.end();
    }
    virtual ~automaton() {}
};

enum { // automata states
    ASInit = 0
};

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

// jpegfuck

struct patch {
	int at;
	uint8_t mask; // to xor
	patch() : at(0), mask(0) {}
	patch(int _at, uint8_t _mask) : at(_at), mask(_mask) {}
};

struct imagefile {
	vector<uint8_t> data;
	vector<patch> patches;
};

struct imagemachine { // Container for image, in case more state is desirable later
	string name;
	imagefile *f;
	texture_slice *rawtexture;
	subtexture_slice *subtexture;
	int pending, ongoing;
	bool displayInfo;
	imagemachine(const string &name);
	void resetTexture();
	virtual ~imagemachine();
};

extern imagemachine *machine;