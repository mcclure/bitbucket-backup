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

#define NAME_OF_THIS_PROGRAM "pongpongpongpongpongpongpongpong"

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
    cpShape *dragging;
    cpVect draggingInto;
    cpVect lastClick;
    dragtouch() : special(not_special), dragging(NULL), draggingInto(cpvzero), lastClick(cpvzero) {}
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
	
	virtual void BackOut() {
		Quit(); // Default implementation just quits.
	}
};
extern type_automaton *cli, *next_cli; // This is an ugly form of what I already do with program_interface
inline void newtype(type_automaton *_cli) { if (cli) cli->die(); cli = _cli; cli->push(); }
inline void nexttype(type_automaton *_cli) { delete next_cli; next_cli = _cli; } // delete b/c never pushed

struct freetype_automaton : public type_automaton {
	int x,y; bool autoinv, cursor;
	freetype_automaton() : type_automaton(), x(0),y(0),autoinv(false), cursor(true) {
	}
	virtual void input(SDL_Event &event);
	void next(int dx, int dy);
	virtual void draw();
};

struct modeset_automaton : public type_automaton {
	bool have_setmode;
	virtual void input(SDL_Event &event);
	void drawmode(int x, int y, bool caps);
};

struct attract_automaton : public modeset_automaton {
	attract_automaton();
	virtual void tick();
	virtual void input(SDL_Event &event);
};

struct dismiss_automaton : public type_automaton {
	dismiss_automaton();
};

struct logo_automaton : public type_automaton {
	int intro_char;
	logo_automaton();
	void blit(int x, int y, int from, int len);
	virtual void tick();
};

struct pong_automaton : public modeset_automaton {
	int next_bullet;
	bool flipped;
	pong_automaton();
	virtual void tick();
	virtual void input(SDL_Event &event);
};

struct failure_automaton : public type_automaton {
	failure_automaton();
	virtual void input(SDL_Event &event);
	virtual void BackOut();
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

#define KHKEYS 9
#define SNAP_BUTTON 8
#define SPECIAL_FIRE_BUTTON 4
#define JHATS 2
#define MAX_PLAYERS 2
struct player_controls {
	int jid;
	bool auto_aim;
	single_control controls[KHKEYS];
	player_controls() : jid(-1), auto_aim(false) {}
};
extern player_controls file_defaults[MAX_PLAYERS], defaults[MAX_PLAYERS];
extern string file_jnames[MAX_PLAYERS];

struct control_handler : public event_automaton {
	int jid;
	bool auto_aim;
	single_control controls[KHKEYS];
	bool axis_based(int c) { return c < 8; }
	control_handler(int _jid = -1) : event_automaton(), jid(_jid) {}
	void load(player_controls &p) {
		jid = p.jid;
		auto_aim = p.auto_aim;
		for(int c = 0; c < KHKEYS; c++) {
			controls[c] = p.controls[c];
		}
	}
	void save(player_controls &p) {
		p.jid = jid;
		p.auto_aim = auto_aim;
		for(int c = 0; c < KHKEYS; c++) {
			p.controls[c] = controls[c];
		}
	}
	single_control &axis_at(int c) { if (c>=2) c+=2; return controls[c]; } // ...this can't go into Jumpcore.
	single_control &hat_at(int c) { return controls[c*4]; }
};

#define DOUBLESNAP 1

#define JHAXES 4
struct player_info;
struct player_handler : public control_handler {
	int sin; // Which sid am I at? TODO: Do live_infos need to know they have an "active" sid?
	int pid;  
	player_info *player, *player_initially;
	int last_fire;
	double axes[JHAXES];
	int axes_at[JHAXES]; // Frames
	bool active[KHKEYS], active_frame[KHKEYS];
	static vector<player_handler *> all_players;
	cpVect last_nonzero_v;
	int last_snap;
	int rtrigger_x, rtrigger_y; // Set on each frame by program.cpp, read by display.cpp
	
	cpFloat camera_zoom;
	cpVect camera_off;
	
	player_handler(player_info *_player, int _sin, int _jid = -1, int _pid = -1) : control_handler(_jid), sin(_sin), pid(_pid), player(_player), player_initially(_player), last_fire(0), last_nonzero_v(cpv(1,0)), last_snap(0), rtrigger_x(0), rtrigger_y(0), camera_zoom(1), camera_off(cpvzero) {
		for(int c = 0; c < JHAXES; c++) {
			axes[c] = 0;
			axes_at[c] = -1;
		}		
		for (int c = 0; c < KHKEYS; c++) {
			active[c] = false;
			active_frame[c] = false;
		}
		all_players.push_back(this);
	}
	void setv(cpVect v);
	virtual void input(SDL_Event &event);
	virtual void tick(); // DOESN'T CALL UPWARD
	void apply_camera(bool reverse = false);
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

#define C_FIXED  0x1
#define C_PLAYER 0x2
#define C_WALL   0x4
#define C_SLIDER 0x8
#define C_SWITCH 0x10
#define C_SEP    0x20
#define C_BUTTON 0x40
#define C_EXIT   0x80
#define C_SOLIDS (C_FIXED|C_PLAYER|C_WALL|C_SLIDER)

#define PNG_P1   0xFF0000FF
#define PNG_P2   0xFF00FF
#define PNG_DROP 0xFFFF

#define GROUP_PREDESTINED 1

#define PLAYER_RAD 0.1

struct live_connector {
	bool attached; // UUNUSED?
	vector<cpShape *> shapes;
	cpBody *body;
	live_connector() : attached(true), body(NULL) {}
};

struct live_info {
	unsigned int collision_type;
	unsigned int collision_interest;
	unsigned int color;
	vector<live_connector> spaces; // Indexed by sid
    live_info(unsigned int _collision_type, unsigned int _collision_interest, unsigned int color = 0xFFFFFFFF) : collision_type(_collision_type), collision_interest(_collision_interest) {}
	virtual ~live_info() {}
	virtual void be_alive_pre(spaceinfo *s) {}
	virtual void be_alive_post(spaceinfo *s) {}
};

struct player_info : public live_info {
	cpVect lastv;
	player_info() : live_info(C_PLAYER, C_SOLIDS|C_SEP), lastv(cpvzero) {}
	static player_info *current_drawing;
	bool drawing_player();
	virtual void be_alive_pre(spaceinfo *s);
};

struct fluttertone;

struct block_info : public live_info {
	block_info() : live_info(C_WALL, C_SOLIDS), dragging(false), lock(0), siren(NULL), wantp(cpvzero), wantp_s(cpvzero), draggingInto(cpvzero), updated(-2), updated_s(-2) {}
	bool dragging;
	unsigned int lock;
	fluttertone *siren;
	cpVect wantp, wantp_s, draggingInto; // "target" position
	int updated; // last updated tick
	int updated_s;
	virtual void be_alive_pre(spaceinfo *s);
};

struct switch_info : public live_info {
	switch_info(unsigned int _collision_type, unsigned int _collision_interest, bool _solid) : live_info(_collision_type, _collision_interest), solid(_solid), updated(-2) {}
	bool solid;
	int updated;
	virtual void be_alive_post(spaceinfo *s);
	static bool button_down();
};

void game_start();
void game_stop();
void game_reset();
void map_init();

// Spaces are "cameras"
struct spaceinfo {
	int sid; // Index in level array
	unsigned int visible; // Is space "visible" (rendered on screen) currently? This is a mask. // UMAYBE?
	cpSpace *space;
	cpBody *staticBody;
	player_info *player[2];
	vector<live_info *> alive;
		
	void tick();
	
	void setup_collisions();
	
	spaceinfo(int _sid) {
		sid = _sid;
		visible = 0;
		space = NULL;
		staticBody = NULL;
	}
};

extern vector<spaceinfo> spaces;

static unsigned int packColor(float r, float g, float b, float a = 1) { // I wish I could separate this more completely.
	unsigned int color = 0;
	unsigned char c;
	c = a*255; color |= c; color <<= 8;
	c = b*255; color |= c; color <<= 8;
	c = g*255; color |= c; color <<= 8;
	c = r*255; color |= c;
	return color;
}

void am_pskew(int mask, int w);
noise *get_kash(int mask);
void am_kash(int mask);
void am_drop(int mask);
void am_drop2(int mask);
void am_pickup(int mask);
void am_hit(int w);
fluttertone * am_flutter(double w);

struct display_idiom {
	bool process; int ppower;
	display_idiom() : process(true),ppower(1) {}
	unsigned int pcode() { return (ppower<<1); }
};

#define DEFAULT_MAXHEALTH 20
#define DEFAULT_STD_ZOOM 0.5
#define DEFAULT_FOREVER_LENGTH 10
#define DEFAULT_SNAP_FACTOR 2
#define DEFAULT_BLOCK_SIZE 0.2
#define DEFAULT_HAVE_CAMERA_FREEDOM 3
#define DEFAULT_HAVE_CAMERA_LIMIT 3
#define DEFAULT_REPEAT 0
#define DEFAULT_PADDED_ROOM 0
#define DEFAULT_IMMORTAL_BULLETS 0
#define DEFAULT_PLAYER_INERTIA (1/5.0)

#define PREVIEW_WIDTH 25
#define PREVIEW_HEIGHT 10
#define PREVIEW_SIZE (PREVIEW_WIDTH*PREVIEW_HEIGHT)
#define DEFAULT_GRAVITY 0
#define DEFAULT_E 1.0
#define DEFAULT_U 0

// Convention for Snap is that if it lives in xml, it's a level idiom.
struct level_idiom {
	string name; // UMAYBE?
	double std_zoom, outside_size, player_inertia; // UMAYBE?
	int have_camera_freedom, have_camera_limit, repeat;//, padded_room, immortal_bullets; // UMAYBE?
	double gravity;
	double w_e, p_e, b_e, w_u, p_u, b_u; // UFIXME
	cpRect camera_visible, camera_nudge, camera_limit;
	level_idiom() : std_zoom(DEFAULT_STD_ZOOM), outside_size(DEFAULT_BLOCK_SIZE), player_inertia(DEFAULT_PLAYER_INERTIA),
	have_camera_freedom(DEFAULT_HAVE_CAMERA_FREEDOM), have_camera_limit(DEFAULT_HAVE_CAMERA_LIMIT),
	repeat(0), gravity(DEFAULT_GRAVITY),
	w_e(DEFAULT_E), p_e(DEFAULT_E), b_e(DEFAULT_E), w_u(DEFAULT_U), p_u(DEFAULT_U), b_u(DEFAULT_E) // b_u is off but should be block
	{}
//	level_idiom *clone(); // UMAYBE?
};

void load_levels_from(string filename);
void new_li(level_idiom *from);
extern vector<level_idiom *> all_levels;
extern int chosen_level, player_count;
extern int wins[2];
void game_reset();
void terminate_game();

extern display_idiom *di;
extern level_idiom *li;
extern double debug_floats[3]; // Nonsense
extern bool game_halt; extern int game_halted_at;
extern bool killAllSound;

extern unsigned int playerColor[4];

#define SQUARE(x) { cpv(-(x),-(x)), cpv(-(x), (x)), cpv( (x), (x)), cpv( (x),-(x)), }
#define RECT(x,y) { cpv(-(x),-(y)), cpv(-(x), (y)), cpv( (x), (y)), cpv( (x),-(y)), }
#define BULLET_DAMAGE 1
#define DPAD_THRESHOLD 0.1

inline const char *S(const char *s) { return s ? s : ""; }