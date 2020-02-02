/*
 *  program.h
 *  Jumpcore
 *
 *  Created by Andrew McClure on 4/27/08.
 *
 */

#include "kludge.h"
#include "controls.h"
#include <math.h>
#include <list>
#include "display.h" // WHATEVER!

#define NAME_OF_THIS_PROGRAM "Jumpcore"

#define PREY_ORIENT_DEBUG 0
#define CURSOR_IS_REAL 0

#if PREY_ORIENT_DEBUG
#define LASTTRAN(x) (x)->reallasttran
#else
#define LASTTRAN(x) (x)->lasttran
#endif

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

void program_init(void);
void program_update(void);

enum touch_type {
    touch_down,
    touch_move,
    touch_up,
    touch_cancel,
};

struct dragtouch {
    enum { not_special = 0, interface_special, } special;
	int button; // Last clicked mouse button. Note: Meaningless on mobile
    cpShape *dragging;
    cpVect draggingInto;
    cpVect lastClick;
    dragtouch() : special(not_special), button(0), dragging(NULL), draggingInto(cpvzero), lastClick(cpvzero) {}
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
	WOptions
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

struct environment;
typedef list<environment *>::iterator environ_iter;
extern list<environment *> environments;

void program_reinit(void);

// Simple specific

#define EC_PLAYER 0xffff00ff
#define EC_WATER 0xffff
#define EC_DEEPWATER 0x7fff
#define EC_FLAME 0xff0000ff
#define EC_WALL 0x000000ff

struct eobject;
typedef list<eobject *>::iterator eobject_iter;

struct eobject {
    struct callback { virtual bool check(int x, int y) = 0; };
    
    int x1, y1, w, h;
    bool xflip;
    unsigned int ec; // type
    inline int x2() { return x1+w-1; }
    inline int y2() { return y1-h+1; } // !!!!
    eobject(int _x1, int _y1, int _w = 0, int _h = 0, unsigned int _ec = 0);
    bool check_corners(callback *c);
};

struct player_eo : public eobject {
    int run_start;
	bool grounded;
	int impetus;
	bool dead; // Replace with rebirth thingy
    player_eo(int _x1, int _y1) : eobject(_x1, _y1, 5, 8, EC_PLAYER), run_start(-1), grounded(true), impetus(0), dead(false) {}
};

// Underlying "world" to represent
struct environment {
    environ_iter anchor;
    void push() {
        environments.push_back(this);
        anchor = --environments.end();
    }   
    virtual void tick() {}
    virtual void input(SDL_Event &event) {}
    list<eobject *> objects;
    environment() {}
};

// "View" of world as grid of grayscale dots
struct dotbox {
    // No environment link cuz like we don't know its class yet?
    int updated; // ticks
    int w,h;
    vector<float> grid;  
    dotbox(int _w = 0, int _h = 0) : updated(0), w(_w), h(_h) { grid.resize(w*h); }
    inline int c(int x, int y) { return y*w+x; }
    virtual void update() {updated = ticks;} // Pull from environment
};

struct blitter;

// Knows how to convert a dotbox into part of a vertex buffer
struct channel {
    dotbox *from;
    virtual void post(blitter *) = 0;
    channel(dotbox *_from = NULL) : from(_from) {}
};

typedef vector<channel *>::iterator channel_iter;

// Collects multiple channels and draws them to screen
struct blitter {
    int w, h;
    float spacing, dotw, dotvary;
    vector<channel *> channels;
    pile backup, grid;
    cpVect base; // 0,0 point. For convenience?
    float free_space; // ALso for convenience?
    int stride, point_off, x_off, y_off, color_off, size_off; // Necessary?
    
    blitter() : w(0), h(0), spacing(0), dotw(0), dotvary(0), base(cpvzero),
        free_space(0), stride(0), point_off(0), x_off(0), y_off(0), color_off(0), size_off(0) {}
    void init(int _w, int _h, float _spacing, float _dotw, float _dotvary);
    void display();
    
    inline int c(int x, int y) { return y*w+x; }
};

extern environment *base_environment;

// Specific environments

struct test_box_environ : public environment {
    int x, y;
	SDLKey arrow_trigger[4];
    bool arrows[4];
	bool jump_pressed;
    int last_moved;
    virtual void tick();
    virtual void input(SDL_Event &event);
    test_box_environ(int _x, int _y) : environment(), x(_x), y(_y), jump_pressed(false), last_moved(0) {
		arrow_trigger[0] = SDLK_DOWN;
		arrow_trigger[1] = SDLK_UP;
		arrow_trigger[2] = SDLK_RIGHT;
		arrow_trigger[3] = SDLK_LEFT;
		for(int c = 0; c < 4; c++) arrows[c] = false;
	}
};

struct level_environment : public test_box_environ {
    slice *l;
    player_eo *player;
    level_environment(int _x, int _y, slice *_level) : test_box_environ(_x,_y), l(_level), player(0) {}
    void init(); // Eat yellow dots
    virtual void tick();
};

// End Simple

enum RunMode {
    run_fly = 0,
    run_game,
    run_max,
    run_normal = run_fly
};

extern RunMode run_mode;

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
    // Simple specific
    virtual void display(blitter *b) {} // Called just before draw
};

enum { // automata states
    ASInit = 0
};

#define DEBUG_VALUE_GON_DEFAULT 4
#define DEBUG_VALUE_GON debug_values[1]

#define DEBUG_VALUE_PACK_DEFAULT 5
#define DEBUG_VALUE_PACK debug_values[2]

#define DEBUG_VALUE_BEND_DEFAULT -1
#define DEBUG_VALUE_BEND debug_values[0]

#define DEBUG_VALUE_STEPFUDGE1 0
#define DEBUG_VALUE_STEPFUDGE2 0

#define DEBUG_VALUE_ARMSPAN_1_1_DEFAULT 0
#define DEBUG_VALUE_ARMSPAN_1_2_DEFAULT 0
#define DEBUG_VALUE_ARMSPAN_1_1 DEBUG_VALUE_ARMSPAN_1_1_DEFAULT
#define DEBUG_VALUE_ARMSPAN_1_2 DEBUG_VALUE_ARMSPAN_1_2_DEFAULT

#define DEBUG_VALUE_ARMSPAN_2_1_DEFAULT 0
#define DEBUG_VALUE_ARMSPAN_2_2_DEFAULT 0
#define DEBUG_VALUE_ARMSPAN_2_1 DEBUG_VALUE_ARMSPAN_2_1_DEFAULT
#define DEBUG_VALUE_ARMSPAN_2_2 DEBUG_VALUE_ARMSPAN_2_2_DEFAULT
#define DEBUG_VALUE_REDRAW_ON(x) (x < 3)

#define DEBUG_VALUE_OUTWARD_DEFAULT 3
#define DEBUG_VALUE_OUTWARD debug_values[6]
#define DEBUG_VALUE_PERARM_DEFAULT 0
#define DEBUG_VALUE_PERARM (loaded->pack-3 + debug_values[7])

#define DEBUG_VALUE_FPA_DEFAULT 10
#define DEBUG_VALUE_FPA DEBUG_VALUE_FPA_DEFAULT

#define OUT_VERBOSE (debug_bools[7])

class bannerstream : public ostringstream {
public:
    int life;
    bannerstream(int _life) : ostringstream() { life = _life; }
    ~bannerstream();
};

// Only use inside brackets!
#define BANNER(z) bannerstream banner(z); banner

#define C_CHIP       0x0001
#define C_CHIP_OUTER 0x0002
#define C_BAR        0x0004
#define C_BAR_TOP    0x1004 // DON'T KEEP
#define C_GUIDE      0x0008
#define C_TOGGLE     0x0010
#define C_ARM        0x0020

#define L_CHIP         C_CHIP
#define L_CHIP_OUTER   0
#define L_BAR          0
#define L_GUIDE        0
#define L_TOGGLE       0
#define L_ARM          0
#define L_WALL         (~0)

extern bool optDebug;

enum MiscButtonType { mb_game_waves, mb_game_tank, mb_game_float, mb_game_color, mb_game_combined };

class MiscButton : public ControlBase { public:
	MiscButtonType t; void *data;
	MiscButton(string _text, MiscButtonType _t, void *_data = NULL) : ControlBase(_text, true), t(_t), data(_data) {}
	virtual void click();
};
extern bool running;
