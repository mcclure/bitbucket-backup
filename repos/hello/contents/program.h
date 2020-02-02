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
#include "cpRect.h"
#include "tinyxml.h"

#define NAME_OF_THIS_PROGRAM "System Disk"

// The following handful of lines are the only ones you can't erase. Feel free to redefine "FPS".

#ifdef TARGET_DESKTOP // This is terrible
#define FPS 80
#else
#define FPS 60
#endif

#define TPF ((int)(1000/FPS))
#define DT (1.0f/FPS)

extern int ticks;
extern int sinceLastFrame;
extern double aspect;
extern bool gotJoystick;
extern bool paused;
extern int surfacew, surfaceh;
cpVect screenToGL(int screenx, int screeny, double desiredZ);

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

typedef void* touch_id;

struct touch_rec {
	touch_id tid;
	cpVect at;
	touch_rec(touch_id _tid, cpVect _at) : tid(_tid), at(_at) {}
};

void program_eventtouch(const list<touch_rec> &touches, touch_type kind);
#endif

void display(void);
void display_init();
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
	WMockupMenu,
	WSetControlsMenu,
	WJoysticksMenu,
	
	WDebugBlur,
};
extern WantNewScreen want; 
extern bool &optWindow;
inline int iabs(int i) { return i < 0 ? -i : i; } // Yeah, I dunno.

void goOrtho();
void orthoText();

struct spaceinfo;
inline bool okayToDraw(spaceinfo *) { return true; }

float textWidth(string str);
float textHeight(string str);

extern string displaying;
extern int displaying_life;

struct automaton; struct event_automaton;
typedef list<automaton *>::iterator auto_iter;
extern list<automaton *> automata;
typedef list<event_automaton *>::iterator eauto_iter;
extern list<event_automaton *> event_automata;

void program_reinit(void);

// Add to this structure if there's some kind of state you want to track alongside each touch/click
struct dragtouch {
	enum { not_special = 0, interface_special } special;
	dragtouch() : special(not_special) {}
};

// manages "animations"
struct automaton {
	int born;  // ticks born at
	int frame; // ticks within state
	int state; // which state are you at?
	int rollover; // what frame does state roll at?
	bool draws;
	auto_iter anchor;
	automaton() : born(ticks), frame(-1), state(0), rollover(1), draws(false) {}
	inline int age() { return ticks-born; }
	virtual void tick();
	virtual void die();
	virtual void draw() {}
	virtual void push() {
		automata.push_back(this);
		anchor = --automata.end();
	}
	virtual ~automaton() {}
};

// "animations" that accept keyboard and joystick events. Used for controller support
// Is this extra work to keep two kinds of automata worth it? Will there even BE any non-event automata?
struct event_automaton : public automaton {
	eauto_iter event_anchor;
	bool halted; // To prevent repeated die() invocations
	event_automaton() : automaton(), halted(false) {}
	virtual void push() {
		automaton::push();
		event_automata.push_back(this);
		event_anchor = --event_automata.end();
	}
	virtual void die(); // DOES NOT CALL UPWARD
	virtual void input(SDL_Event &event) {}
};

struct type_automaton : public event_automaton {
	unsigned char screen[24][40]; // NOTE: Y,X
	unsigned int  scolor[24][40]; // NOTE: Y,X // I bet this could be optional.
	void clear() {
		memset(screen, ' ', sizeof(screen));
		memset(scolor, 0xFF, sizeof(scolor));
	}
	type_automaton() : event_automaton() {
		clear();
		draws = true;
	}
	virtual void draw();
	void set(int x, int y, unsigned char c, bool inv = false);
	void set(int x, int y, string s, bool inv = false);
	void set_centered(int x, int y, int w, string s, bool clearfirst = true, bool inv = false);
	void load_text(string filename);
	void save_text(string filename);
	void dump(); // Debug
	virtual ~type_automaton();
	void scroll();
	
	virtual void BackOut() {
		Quit(); // Default implementation just quits.
	}
};
extern type_automaton *cli, *next_cli; // This is an ugly form of what I already do with program_interface
inline void newtype(type_automaton *_cli) { if (cli) cli->die(); cli = _cli; cli->push(); }
inline void nexttype(type_automaton *_cli) { delete next_cli; next_cli = _cli; } // delete b/c never pushed

// Note: In DOS 3.3 mode, "draw" from this is used, everything else gets overridden
struct freetype_automaton : public type_automaton {
	int x,y; bool autoinv, cursor;
	freetype_automaton() : type_automaton(), x(0),y(0),autoinv(false), cursor(true) {
	}
	virtual void input(SDL_Event &event);
	void next(int dx, int dy);
	virtual void draw();
};

struct joystick {
	string name;
	joystick(string _name) : name(_name) {}
};
extern vector<joystick> joysticks;
extern hash_map<string, int> joysticks_by_name;

struct single_control {
	SDLKey key;
	int button; // Like joystick button
	int axis;
	int hat;
	bool is_key() { return key != 0; }
	bool is_button() { return button >= 0; }
	bool is_axis() { return axis >= 0; } 
	bool is_hat() { return hat >= 0; }
	bool is_anything() { return is_key() || is_button() || is_axis() || is_hat(); } // FIXME: Keep updating
	single_control() : key((SDLKey)0), button(-1), axis(-1), hat(-1) {}
	bool operator==(const single_control &o) const {
		return key==o.key&&button==o.button&&axis==o.axis&&hat==o.hat;
	}	
	bool operator!=(const single_control &o) const {
		return !(*this==o);
	}
	string readable();
};

#define KHKEYS 0
#define SNAP_BUTTON 8
#define SPECIAL_FIRE_BUTTON 4
#define JHATS 2
#define MAX_PLAYERS 0
struct player_controls {
	int jid;
	bool auto_aim;
	single_control controls[KHKEYS];
	player_controls() : jid(-1), auto_aim(false) {}
};
extern player_controls file_defaults[MAX_PLAYERS], defaults[MAX_PLAYERS];
extern string file_jnames[MAX_PLAYERS];

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

// Does this stuff need to be globally visible?
extern pthread_mutex_t audio_mutex;
struct noise;
struct bgaudio { // TODO start at particular time?
	noise *s;
	bool back, discard_wrapper;
	bgaudio(noise *_s, bool _back = false, bool _discard_wrapper = false) : s(_s), back(_back), discard_wrapper(_discard_wrapper) {}
	void insert(); // Call from main thread
	void stop(); // Call from main thread
};
extern vector<bgaudio *> bgaudio_loose;
typedef vector<bgaudio *>::iterator bgaudio_iter;
void bgaudio_reset();

void load_controls();
void push_controls(int);
void save_controls();

// End Jumpcore basic stuff-- add your own program state below

static unsigned int packColor(float r, float g, float b, float a = 1) { // I wish I could separate this more completely.
	unsigned int color = 0;
	unsigned char c;
	c = a*255; color |= c; color <<= 8;
	c = b*255; color |= c; color <<= 8;
	c = g*255; color |= c; color <<= 8;
	c = r*255; color |= c;
	return color;
}

struct display_idiom {
	bool process; int ppower;
	display_idiom() : process(true),ppower(1) {}
	unsigned int pcode() { return (ppower<<1); }
};

extern display_idiom *di;

// DOS 3.3

struct basic_automaton; struct input_mode; struct line_op;

// Modes are something that can "take over"-- they can tick or take input
struct mode_interface {
	basic_automaton *parent;
	bool zombie; // What are zombies, I forget
	mode_interface(basic_automaton *_parent = NULL) : parent(_parent), zombie(false) {}
	virtual void tick() {}
	virtual void enter() { ERR("ASSERT?! SHOULDN'T GET HERE.\n"); }
	virtual void key(SDLKey sym, Uint16 unicode, Uint8 type) {} // Type is SDL_KEYDOWN, SDL_KEYUP or 0 for repeat
};

// Ops are freezedried "commands". They can act as modes
struct op_interface : public mode_interface {
	mode_interface *next;
	
	string bufout;
	mode_interface *bufnext;
	
	op_interface(basic_automaton *_parent) : mode_interface(_parent), next(NULL), bufnext(NULL) {}
	virtual void tick();
	virtual void key(SDLKey sym, Uint16 unicode, Uint8 type);
	void done(mode_interface *into = NULL);
	void done(const string &data, mode_interface *into = NULL);
	void print(const string &data);
	virtual void exit();
	virtual void enter() { done(); }
	virtual string toString() = 0;
	static int cycles;
	static void reset_cycles();
};

struct exp_interface {
	virtual string strval() = 0;
	virtual string toString() = 0;
};

// Inherits freetype, but doesn't use autoinv (does use cursor)
struct basic_automaton : public freetype_automaton {
	mode_interface *mode;
	input_mode *std_input;
	line_op *root;
	
	basic_automaton();
	virtual void input(SDL_Event &event);
	virtual void tick();
	void lf();
	void next(int dx, int dy);
	void nextmode(mode_interface *next);
	op_interface *parse(const string &entry);
	
	line_op *line_before(int line);
	line_op *line_at(int line);
};