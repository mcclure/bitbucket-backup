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
#include "tinyxml.h"
#include "dclib.h"

#define NAME_OF_THIS_PROGRAM "Drumcircle"

// The following handful of lines are the only ones you can't erase. Feel free to redefine "FPS".

#ifdef TARGET_DESKTOP // This is terrible
#define FPS 80.0
#else
#define FPS 60.0
#endif

#define TWIST1_DEBUG 0
extern void twisthit(cpVect);

#define TPF ((int)(1000/FPS))
extern int ticks;
extern int sinceLastFrame;
extern double aspect;
extern bool gotJoystick;
extern bool paused;
extern int surfacew, surfaceh;
extern bool phoneFactor;
cpVect screenToGL(int screenx, int screeny, double desiredZ);

extern double glPixel, chipSize, chipSizePlus;
#define BASE_CHIP 0.0333


extern cpSpace *uiSpace;
inline cpSpace *workingUiSpace() { return uiSpace; }

void recreate_surface(bool in_a_window);

// CALLBACKS

void BackOut();
void AboutToQuit();
void Quit(int code = 0);
void BombBox(string why = string());
void FileBombBox(string filename = string());

void program_init(void);
void program_update(void);

#ifdef TARGET_MOBILE
void program_sleep();
void program_wake();
#endif

bool interface_attempt_click(cpVect screen_coord, int button);

enum touch_type {
    touch_down,
    touch_move,
    touch_up,
    touch_cancel,
};

#ifdef TARGET_DESKTOP
void program_eventkey(SDL_Event &event);
void program_eventjoy(SDL_Event &event);
void program_eventmouse(SDL_Event &event);
#else

// Mini generic

uint64_t SDL_GetTicks();

#ifdef TARGET_ANDROID
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
	WMainMenu,
	WOnlineConnect,
	WSettings,
	WDisk,
	WDiskConfirm,
	WSaveWav,
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

// Debug

extern int debug_values[9];
extern int debug_floating[3];
extern float debug_floats[3];
#define DEBUG_BOOLS_COUNT 10
extern bool debug_bools[DEBUG_BOOLS_COUNT];
extern int debug_masterint;
extern int debug_stamp, debug_stampres;
extern void *debug_stamp_id;
extern slice *debug_numbers_slice;
extern texture_slice *debug_display_slice;

struct automaton;
typedef list<automaton *>::iterator auto_iter;
extern list<automaton *> automata;

void program_reinit(void);

void wav_write(const char *filename);

#define JOB_NOTHREAD 0

// Automatons are long-running state machines that act on every frame.
// Jobs are single-use tasks that run in a separate thread.
// TODO: Result/status code or intptr_t or somesuch?
struct job {
	bool done; // Should always be true by end of execute()
	void wait(); // Block until done. Obviously call only if  TODO: Needs work, see .cpp
	void insert(); // In JOB_NOTHREAD mode, will execute right away
	virtual void execute() = 0;
	// "Halt" is used for best-effort cleanup, if any, when program quits. 
	virtual bool want_halt() { return false; } // Called from main thread, WITH lock held. If true, will call:
	virtual void halt() {}					   // Called from main thread, WITHOUT lock held.
	job() : done(false) {}
	~job() {}
};

typedef list<job *>::iterator job_iter;
extern list<job *> job_current; // Only modify while holding job_mutex
extern list<job *> job_discard; // Jobs which will be deleted WHEN DONE. Only to be modified by main thread.
#if !JOB_NOTHREAD
extern pthread_mutex_t job_mutex;
#endif
void jobs_init(); // Call at startup
void jobs_service(); // Call during update-- ATM just clears out _discard
void jobs_terminate(); // Kill all jobs. To be called at program quit.

// Drumcircle specific

extern int toggle_fire_at;

extern bool display_board, board_obscured, board_loading;

sample_inst *library_check(const char *name, int pmod, const char *img);

extern bool metro_dirty;
extern cpVect metro_at_drawn;
extern bool metro_special_happening;

struct dragtouch;

struct guichip : chip {
    vector<cpShape *> shapes; // Necessary?
	int twistStartAt;
	bool oversize;
#ifdef TARGET_MOBILE
	dragtouch *twist;
#endif	
    guichip();
    void readjust(cpVect to);
	void twistStart();
	void twistUpdate(cpFloat to);	
};

struct dragtouch {
	// No special designation; eaten by interface; eaten by metronome; eaten by [something that means it can't twist]
    enum { not_special = 0, interface_special, metro_special, untwist_special } special;
	int button; // Last clicked mouse button. Note: Meaningless on mobile
    cpShape *dragging;
    cpVect draggingInto;
    cpVect lastClick;

#ifdef TARGET_MOBILE
	cpShape *twistDrag;
	dragtouch *twistPartner;
	cpFloat twistLast;
	cpFloat twistOff;
	void updateTwistLast();
	void updateTwistOff(); // calls updateTwistLast
	void forgetTwist();
#endif
    dragtouch() : special(not_special), button(0), dragging(NULL), draggingInto(cpvzero), lastClick(cpvzero)
#ifdef TARGET_MOBILE
		, twistDrag(NULL), twistPartner(NULL),twistOff(0)
#endif
	{}
};

inline cpFloat vectToTime(cpVect v) {
    return fmod(1-cpvtoangle(v)/(2*M_PI),1.0);
}

inline cpVect timeToVect(cpFloat t) {
    return cpvforangle(-t*M_PI*2);
}

#define C_CHIP       0x0001
#define C_CHIP_OUTER 0x0002
#define C_BAR        0x0004
#define C_GUIDE      0x0008
#define C_TOGGLE     0x0010
#define C_ARM        0x0020

// TODO: Set all groups same, use layer search in Chipmunk 5 instead of doing it by hand
#define L_CHIP         C_CHIP
#define L_CHIP_OUTER   C_CHIP_OUTER
#define L_BAR          C_BAR
#define L_GUIDE        C_GUIDE
#define L_TOGGLE       C_TOGGLE
#define L_ARM          C_ARM
#define L_WALL         (~0)

// A4 = 44100/440 = 100.22727...
#define MIDDLEA   100 
#define NOTE_BASE (-19)
#define BARHEIGHT (1/10.0)
const extern double halfstep;

enum {
    ibar_esc = 0,
    ibar_save,
    ibar_load,
    ibar_reset,
    ibar_dupe,
    ibar_grid,
    ibar_settings,
    ibar_max
};

struct barbutton {
    subtexture_slice *s;
    unsigned int color;
    barbutton(subtexture_slice *_s = NULL, unsigned int _color = 0xFFFFFFFF) : s(_s), color(_color) {}
    virtual cpShape* click(cpVect) {return NULL;} // Returned value overwrites draggingMe. Default does nothing
};

struct generator_bb : public barbutton {
    chip_payload ch;
    generator_bb(chip_payload _ch);
    virtual cpShape* click(cpVect);
};

struct ibar_bb : public barbutton {
    unsigned int t;
    ibar_bb(subtexture_slice *_s, unsigned int _color, unsigned int _t) : barbutton(_s,_color), t(_t) {}
    virtual cpShape* click(cpVect);
};

struct bardata : public bodydata {
	virtual barbutton * button(int i) = 0;
	virtual int button_count() = 0;
	bardata() : bodydata(C_BAR) {}
};

struct onebardata : public bardata {
    vector<barbutton *> buttons;
	virtual barbutton * button(int i) { return buttons[i]; }
	virtual int button_count() { return buttons.size(); }
    onebardata() : bardata() {}
};    

struct multibardata : public bardata {
    vector<onebardata *> bars;
	int active;
	virtual barbutton * button(int i) { return bars[active]->button(i); }
	virtual int button_count() { return bars[active]->button_count(); }
    multibardata() : bardata(), active(0) {}
};    

struct barop_bb : public barbutton { // Too simplistic?
	multibardata *b;
	barop_bb(subtexture_slice *_s, unsigned int _color, multibardata *_b) : barbutton(_s,_color), b(_b) {}
	virtual cpShape *click(cpVect) { b->active++; b->active %= b->bars.size(); return NULL; }
};

struct netxml_job : public job {
	bool have_send;
	TiXmlDocument send, recv;
	char *xml_update_url_copy; // http_parse_url will devour this
	netxml_job(const string &_xml_update_url) : job(), have_send(false) {
		xml_update_url_copy = strdup(_xml_update_url.c_str());
	}
	virtual void execute();
	virtual bool want_halt() { return true; }
	virtual void halt() { wait(); } // Must complete; no other options
	virtual ~netxml_job() { free(xml_update_url_copy); }
};

// End drumcircle

enum RunMode {
    run_fly = 0,
    run_game,
    run_max,
    run_normal = run_fly
};

extern bool do_grid, do_dupe, do_sync;
extern RunMode run_mode;
extern int key_off;
extern bool doPlayFeedback, doTextFeedback, doLetters;

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

#define DEBUG_VALUE_REDRAW_ON(x) (x < 3)

class bannerstream : public ostringstream {
public:
    int life;
    bannerstream(int _life) : ostringstream() { life = _life; }
    ~bannerstream();
};

// Only use inside brackets!
#define BANNER(z) bannerstream banner(z); banner

extern bool optDebug;

// TODO move back to display.h before Jumpcore release
static unsigned int packColor(float r, float g, float b, float a = 1) {
	unsigned int color = 0;
	unsigned char c;
	c = a*255; color |= c; color <<= 8;
	c = b*255; color |= c; color <<= 8;
	c = g*255; color |= c; color <<= 8;
	c = r*255; color |= c;
	return color;
}

#define VERSION "1.0.1b"
#if defined(WINDOWS)
#define PLATFORM "-w"
#elif defined(TARGET_IPHONE)
#define PLATFORM "-i"
#elif defined (TARGET_MAC)
#define PLATFORM "-m"
#elif defined (LINUX)
#define PLATFORM "-x"
#elif defined(TARGET_ANDROID)
#define PLATFORM "-a"
#elif defined(TARGET_WEBOS)
#define PLATFORM "-p"
#endif
#define FULLVERSION (VERSION PLATFORM)

#if defined(TARGET_IPHONE) || defined(TARGET_ANDROID)
#define DO_HELPBUTTON
#endif
void open_url(string url);
