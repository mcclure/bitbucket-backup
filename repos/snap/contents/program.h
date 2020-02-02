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

#define NAME_OF_THIS_PROGRAM "Snap"

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

struct attract_automaton : public type_automaton {
	static int rollovers[2];
	unsigned char backscreen[2][24][40];
	attract_automaton();
	virtual void input(SDL_Event &event);
	virtual void tick();
};

// Level select
struct level_automaton : public type_automaton {
	unsigned char backscreen[24][40];
	int selected, last_drawn, draw_off;
	level_automaton(int initial_selected=0);
	virtual void input(SDL_Event &event);
	virtual void tick();
	virtual void BackOut();
};

struct logo_automaton : public type_automaton {
	int intro_char;
	logo_automaton();
	void blit(int x, int y, int from, int len);
	virtual void tick();
};

struct restart_automaton : public type_automaton {
	restart_automaton();
	virtual void input(SDL_Event &event);
	virtual void BackOut();
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

struct player_screen : public type_automaton {
	int target; // currently in-use control (if any, see state)
	control_handler *controls[2]; // Note: will leak if player_screen is deleted before die
#if DOUBLESNAP
	bool ready[2];
#endif
	player_screen();
	void reset(int x);
	void set_control(int x);
	void set_boxtext(int x, int y, string s, bool inv=false); // y is relative to top of bottom cell
	void set_feedback(int x, int y); // y is line not literal y
	virtual void push() { type_automaton::push(); controls[0]->push(); controls[1]->push(); }
	virtual void die() { type_automaton::die(); controls[0]->die(); controls[1]->die(); }
	virtual void input(SDL_Event &event);
	virtual void BackOut();
};

#define JHAXES 4
struct player_info;
struct player_handler : public control_handler {
	int sin; // Which sid am I at? TODO: Do live_infos need to know they have an "active" sid?
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
	
	player_handler(player_info *_player, int _sin, int _jid = -1) : control_handler(_jid), sin(_sin), player(_player), player_initially(_player), last_fire(0), last_nonzero_v(cpv(1,0)), last_snap(0), rtrigger_x(0), rtrigger_y(0), camera_zoom(1), camera_off(cpvzero) {
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
	void setf(cpVect v);
	virtual void input(SDL_Event &event);
	virtual void tick(); // DOESN'T CALL UPWARD
	void apply_camera();
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

#define C_WALL 0x1
#define C_PLAYER 0x2
#define C_BULLET 0x4
#define C_HEALTH 0x8

#define PNG_P1   0xFF0000FF
#define PNG_P2   0xFF00FF
#define PNG_DROP 0xFFFF

#define GROUP_PREDESTINED 1

struct f_event;
struct depend {
	f_event *on; // "Child"
	int at; // Require parent to survive to this frame, or -1 for instantaneous. If positive assume f_lifetime cast for parent is safe
	int after; // If fail, child fails at this moment. If positive assume f_lifetime/f_quantity cast for child is safe
	depend(f_event *_on = NULL, int _at = -1, int _after = -1) : on(_on), at(_at), after(_after) {}
};
struct turnoff {
	int is; // Halt at this frame
	depend cause;
};

#define TRACK_EVENT_PARENTS 0 // Actually, does it matter? Can events have more than one parent?

// Something a "fork" can depend on
struct f_event {
	int occur;
	bool canon; // "in continuity"? TODO: make a reference count?
#if TRACK_EVENT_PARENTS
	vector<depend> parents; // I am canon if
#endif
	vector<depend> children; // I determine canon of
	virtual void fire(spaceinfo *) {}
	f_event(int _occur) : occur(_occur), canon(true) {}
	void make_depend(f_event *on, int at = -1, int after = -1);
	virtual void set_canon(bool is, int after = -1);
};

struct f_lifetime : public f_event {
	int max; // // This is a frame #, not an age
	vector<turnoff> turnoffs;
	bool open_end; // Misnamed-- proxy for "alive"
	bool soft_launch; // Move to object_lifetime?
	int ends() { return max; }
	virtual void fire(spaceinfo *);
	virtual void halt(spaceinfo *);
	f_lifetime(int _occur, int _max) : f_event(_occur), max(_max), soft_launch(false), open_end(true) {}
	virtual void be_alive_pre(spaceinfo *) {}
	virtual void be_alive_post(spaceinfo *) {}
	virtual f_lifetime *reincarnate(spaceinfo *from, spaceinfo *to, int t); // Creates an object with "continuity". Note: NOT a clone.
	virtual void draw(spaceinfo *) {} // TODO: *Should* all lifetimes be visible? What about statics?
};
struct f_quantity : public f_event { // Doubles as both a record of a quantity and a change in that quantity.
	int value;		// Current value
	int increment;	// Delta at this event-- added to value iff canon
	f_quantity *next; // Quantities have clear continuity
	f_quantity(int _occur, int _before, int _increment, f_quantity *_next = NULL) : f_event(_occur), value(_before), increment(_increment), next(_next) {}
	virtual f_quantity *reincarnate(int occur, int new_increment, bool contemporary); // Insert a new thing in the chain
	virtual void propagate();
};

// Anything that can live "in" a timeline
// TODO: Establish clear guidelines for when body is/isn't filled in
struct live_connector {
	bool attached;
	vector<cpShape *> shapes;
	cpBody *body;
	live_connector() : attached(false), body(NULL) {}
};
struct object_lifetime;
struct live_info {
	unsigned int collision_type;
	unsigned int collision_interest;
	vector<live_connector> spaces; // Indexed by sid
	vector<cpVect> p; vector<cpVect> v; // Indexed by time since birth. "Active" == "More slots left"
	object_lifetime *life; // 1 to 1 right?
    live_info(unsigned int _collision_type, unsigned int _collision_interest) : collision_type(_collision_type), collision_interest(_collision_interest), life(NULL) {}
	virtual ~live_info() {}
	virtual live_info *reincarnate(spaceinfo *, spaceinfo *, int) { return new live_info(collision_type, collision_interest); }
	virtual void spawn(spaceinfo *) {}
};

#define PLAYER_RAD 0.1

struct health_quantity;
typedef list<health_quantity *>::iterator health_iter;

struct player_info : public live_info {
	unsigned int mask, color, shotw;
	cpVect lastv;
	list<health_quantity *> lifebar;
	player_info() : live_info(C_PLAYER, C_WALL|C_BULLET|C_HEALTH), color(0xFFFFFFFF), shotw(30), lastv(cpvzero) {}
	virtual live_info *reincarnate(spaceinfo *from, spaceinfo *to, int t);
	static player_info *current_drawing;
	bool drawing_player();
	inline health_quantity *health() { return *lifebar.rbegin(); }
};

#define BULLET_RAD 0.025
struct bullet_info : public live_info {
	player_info *owner;
	bullet_info() : live_info(C_BULLET, C_WALL|C_PLAYER|C_HEALTH), owner(NULL) {}
	virtual live_info *reincarnate(spaceinfo *from, spaceinfo *to, int t);
	virtual void spawn(spaceinfo *);
	void land(spaceinfo *, player_info *);
};

#define DROP_RAD PLAYER_RAD
struct drop_info : public live_info {
	int expire;
	drop_info(int whatami);
	virtual void spawn(spaceinfo *);
	virtual live_info *reincarnate(spaceinfo *from, spaceinfo *to, int t);
};

void map_init(int players);

typedef hash_map<void *, f_lifetime *>::iterator lifetime_iter;

// Spaces are "cameras"
struct spaceinfo {
	int sid; // Index in level array
	int time_off; // Relative to "now"
	unsigned int visible; // Is space "visible" (rendered on screen) currently? This is a mask.
	cpSpace *space;
	cpBody *staticBody;
	
	hash_map<void *, f_lifetime *> present; // Objects currently on screen
	hash_map<void *, f_lifetime *> alive; // Objects currently updated
	vector<f_lifetime *> dying_this_frame; // Todo: Figure out a way around this...?
	
	int time();
	
	void tick();
	
	void setup_collisions();
	
	spaceinfo(int _sid, int _time_off) {
		sid = _sid;
		time_off = _time_off;
		visible = 0;
		space = NULL;
		staticBody = NULL;
	}
};

extern vector<spaceinfo> spaces;

struct moment {
	vector<f_event*> events;
//	vector<live_info *> ever_objects; // TODO
	void enter(spaceinfo *);
	void leave(spaceinfo *);
};

struct forever {
	int period; // Length of time
	int snap; // Size of snap
	int frame; // What frame is the world at right now? Is a global time even a good idea?
	int lastdrop, lastdropat;
	vector<moment> moments;
	void init(int _period, int _snap);
};

extern forever now;

struct object_lifetime : public f_lifetime {
	live_info *watch;
	virtual void fire(spaceinfo *); // Soft enter
	virtual void halt(spaceinfo *); // Soft exit
	object_lifetime(live_info *_watch, int _occur) : f_lifetime(_occur, now.period), watch(_watch) { // Objects live forever
		watch->life = this;
	}
	virtual void be_alive_pre(spaceinfo *);
	virtual void be_alive_post(spaceinfo *);
	virtual void draw(spaceinfo *);
	virtual f_lifetime *reincarnate(spaceinfo *from, spaceinfo *to, int t); // Won't t always be 0?
	void remove_free_will(int at);
};

struct health_quantity : public f_quantity {
	player_info *owner;
	health_quantity(player_info *_owner, int _occur, int _oldhealth, int _increment, f_quantity *_next = NULL) : f_quantity(_occur,_oldhealth,_increment,_next), owner(_owner) {}
	virtual f_quantity *reincarnate(int occur, int new_increment, bool contemporary); // Insert a new thing in the chain
	virtual void propagate();
	virtual void set_canon(bool is, int after = -1);
};

struct audio_event : public f_event {
	noise *n;
	virtual void fire(spaceinfo *);
	audio_event(int _occur, noise *_n) : f_event(_occur), n(_n) {} 
};

struct animate_lifetime : public f_lifetime {
	noise *n;
	int off, upto;
	animate_lifetime(int _occur, int _upto, int _off, noise *_n = NULL) : f_lifetime(_occur, ::min(now.period,_occur + _upto - _off)), n(_n), off(_off), upto(_upto) {
		open_end = occur+upto > now.period; // FIXME: Rethink how I got into this position :/
	}
	int frame(spaceinfo *s) { return s->time() - occur + off; }
	virtual void fire(spaceinfo *);
	virtual f_lifetime *reincarnate(spaceinfo *from, spaceinfo *to, int t) = 0; // Force abstract
};

struct kash_animate : public animate_lifetime {
	cpVect p;
	kash_animate(int _occur, cpVect _p, int _off, noise *_n) : animate_lifetime(_occur, 10, _off, _n), p(_p) {}
	virtual void draw(spaceinfo *);
	virtual f_lifetime *reincarnate(spaceinfo *from, spaceinfo *to, int t);
};

#define SNAP_FADEIN_FRAMES 10
struct snap_animate : public animate_lifetime {
	player_info *follow;
	snap_animate(int _occur, player_info *_follow, int _off) : animate_lifetime(_occur, SNAP_FADEIN_FRAMES, _off, NULL), follow(_follow) {}
	virtual void draw(spaceinfo *);
	virtual f_lifetime *reincarnate(spaceinfo *from, spaceinfo *to, int t);
};

struct gameover_event : public f_event {
	int deathmask;
	gameover_event(int _occur, int _deathmask) : f_event(_occur), deathmask(_deathmask) {}
	virtual void fire(spaceinfo *);
};

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
void am_snap(int mask,int,int);

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

#define PREVIEW_WIDTH 25
#define PREVIEW_HEIGHT 10
#define PREVIEW_SIZE (PREVIEW_WIDTH*PREVIEW_HEIGHT)
#define DEFAULT_GRAVITY 0
#define DEFAULT_E 0.8
#define DEFAULT_U 0

// Convention for Snap is that if it lives in xml, it's a level idiom.
struct level_idiom {
	string name; unsigned char preview[PREVIEW_SIZE];
	block_slice *s; vector<cpVect> spawners[3];
	int maxhealth; double std_zoom; double forever_length; int snap_factor; double block_size;
	int have_camera_freedom, have_camera_limit, repeat, padded_room, immortal_bullets; double gravity;
	double w_e, p_e, b_e, w_u, p_u, b_u;
	cpRect camera_visible, camera_nudge, camera_limit;
	double d_p[3], d_a[3]; int d_q[3];
	string message;
	level_idiom() : s(NULL), maxhealth(DEFAULT_MAXHEALTH), std_zoom(DEFAULT_STD_ZOOM), forever_length(DEFAULT_FOREVER_LENGTH),
	snap_factor(DEFAULT_SNAP_FACTOR), block_size(DEFAULT_BLOCK_SIZE),
	have_camera_freedom(DEFAULT_HAVE_CAMERA_FREEDOM), have_camera_limit(DEFAULT_HAVE_CAMERA_LIMIT),
	repeat(DEFAULT_REPEAT), padded_room(DEFAULT_PADDED_ROOM), immortal_bullets(DEFAULT_IMMORTAL_BULLETS), gravity(DEFAULT_GRAVITY),
	w_e(DEFAULT_E), p_e(DEFAULT_E), b_e(DEFAULT_E), w_u(DEFAULT_U), p_u(DEFAULT_U), b_u(DEFAULT_E) // b_u is off but I forget why
	{
		memset(preview, ' ', PREVIEW_SIZE);
		memset(d_p, 0, sizeof(d_p));
		memset(d_a, 0, sizeof(d_a));
		memset(d_q, 0, sizeof(d_q));
	}
	void load(TiXmlElement *e);
	bool load_preview(string filename);
	level_idiom *clone();
	int starthealth() { return maxhealth; }
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
extern unsigned int show_dead;

extern unsigned int playerColor[4];
extern unsigned int playerShotw[2];

#define SQUARE(x) { cpv(-(x),-(x)), cpv(-(x), (x)), cpv( (x), (x)), cpv( (x),-(x)), }
#define BULLET_DAMAGE 1
#define DPAD_THRESHOLD 0.1
