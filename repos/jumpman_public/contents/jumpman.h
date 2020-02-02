/*
 *  jumpman.h
 *  Jumpman
 *
 *  Created by Andi McClure on 4/27/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "controls.h"
#include <math.h>
#include <list>

// CONSTANTS

// These correspond, perhaps foolishly, to colors.
#define C_JUMPMAN  9000ul
#define C_RESIZE   9100ul
#define C_RESIZE2  9110ul
#define C_LINE     9120ul
#define C_SCROLL   9130ul
#define C_OUTLINE  9200ul
#define C_OUTLINE2 9300ul
#define C_UNKNOWN  9400ul
#define C_BIRD     9500ul

#define C_NOTHING 0xFFFFFFFF
#define C_FLOOR   0x000000FF
#define C_LAVA    0xDDDDDDFF
#define C_NOROT	  0xAAAAAAFF
#define C_LOOSE	  0x111111FF
#define C_LOOSE2  0x111112FF
#define C_LOOSE3  0x111113FF
#define C_BOMB	  0x00FFFFFF
#define C_BOMB2	  0x00AAAAFF
#define C_EXIT    0x0000FFFF
#define C_BACK    0x000011FF
#define C_PAINT   0x0000AAFF
#define C_ARROW   0x0000DDFF
#define C_MARCH   0xFF0000FF
#define C_INVIS   0xFFFF00FF
#define C_ENTRY	  0xFF7F00FF
#define C_REENTRY 0xFF7F7FFF
#define C_ING	  0xFF0040FF
#define C_STICKY  0xFF0080FF
#define C_SWOOP	  0xFF00B0FF
#define C_BALL	  0x00FF00FF
#define C_HAPPY	  0x00DD00FF
#define C_ANGRY	  0x00AA00FF

// TODO: Figure out what I'm doing with the mask fields
#define C_MASK    0xFFFFFF00
#define C_META    0x000000FF
#define C_RMASK   0x000000FC

#define C_FLOORTYPE(x) ((x) == C_FLOOR || (x) == C_LAVA || (x) == C_LOOSE || (x) == C_LOOSE2 || (x) == C_NOROT || (x) == C_REENTRY)

#define C_INVISTYPE(x) ((x) == C_INVIS || (x) == C_LOOSE3)
#define C_SYMTYPE(x) (C_FLOORTYPE(x) || C_INVISTYPE(x))
#define C_WRAPTYPE(x) ((x) == C_FLOOR || (x) == C_LAVA || C_INVISTYPE(x) || (x) == C_LOOSE2 || (x) == C_EXIT || (x) == C_NOROT)
#define C_BALLTYPE(x) ((x) == C_BALL || (x) == C_HAPPY || (x) == C_ANGRY)

// TODO: Speed should be dynamic?

#define VSYNC_DESPERATION 0
#define FPS 80.0
#define TPF ((int)(1000/FPS))

#define MAXLAYERS 32

#define COMPILED_VERSION 1.0

#define DOING_VIDEOS 0

#if DOING_VIDEOS
struct eventHappened {
	bool isEvent;
	SDL_Event event;
	eventHappened() : isEvent(false) {}
	eventHappened(SDL_Event &anEvent) : event(anEvent), isEvent(true) {}
};
void videoStart(string filename);
void videoFrame();
void videoEvent(SDL_Event &);
void videoEnd();
void videoPlayback(bool tofile = true);
void videoSave();
void videoLoad();
#else
#define videoStart(filename)
#define videoFrame()
#define videoEvent(n)
#define videoEnd()
#endif

// LEVELS

enum camera_type {
	cam_track = 0,
	cam_fixed
};

enum repeat_type {
	repeat_no = 0,
	repeat_normal 
};

enum platebehave {
	nobehave = 0,
	manbehave,
	addbehave
};
struct cloneable { // A little bitty factory for void *datas
	virtual cloneable *clone() { return new cloneable(); }
};

struct plateinfo {
	int width, height;
	int frames;
	float r, g, b;
	cpVect constv; float u;
	platebehave behave;
	slice **slices;
	unsigned int *displays;
	cloneable *defaultdata;
	plateinfo() {
		width = 0; height = 0; frames = 0; displays = 0;
		r = 1; g = 1; b = 1;
		constv = cpvzero; u = 0.5;
		behave = nobehave;
		slices = NULL;
		defaultdata = NULL;
	}
	~plateinfo() {
		if (displays)
			delete[] displays;
		if (slices) {
			for(int c = 0; c < frames; c++)
				delete slices[c];
			delete[] slices;
		}
	}
	void reconstruct();
};

struct spaceinfo;

struct splode_info {
	slice *sploding;
	double r, g, b;
	cpVect p;
	bool reflect;
	int ct, tt;
	splode_info(spaceinfo *space, cpShape *shape, int frame, cpVect _p, bool _reflect);
	void adjust_sound();
};

struct trail_info {
	cpVect at, trail;
	double r, g, b;
	int ct, tt;
	trail_info(cpVect _at, cpVect _trail, double _r, double _g, double _b);
};

struct enemy_info : public cloneable {
	int lastWhichFrame;
	bool lastReflect;
	bool noRot;
	int tiltid; // or -1 for mans
	bool tiltref;
	cpVect tiltg;
	
	enemy_info(bool _noRot = false) { lastWhichFrame = 0; lastReflect = false; noRot = _noRot; tiltid = 0; tiltref = false; tiltg = cpv(1,0);}
	virtual cloneable *clone() { return new enemy_info(noRot); } // Remember, C++ won't forward this in children...
};

#define REPROTROT 3
#define REPROTREF 4
#define REPROTALL 7

struct spaceinfo {
	double r[MAXLAYERS], g[MAXLAYERS], b[MAXLAYERS];
	int num;
	int deep;

	int layers;
	camera_type camera;
	repeat_type repeat;
	
	bool reprot;	// Do we rotate on repeat?
	unsigned int reprots; // How do we rotate on repeat?
	char disprot[4][4];
	cpVect master_tiltg;
	
	double repeat_every;
	unsigned char rots, orots; // It's a bitmask! I am so sorry!
	unsigned char dontrot, odontrot; // It's also a bitmask!
	int rspeed;
	double zoom;
	int after; // Will be subtracted from the next "deep".
	
	bool haveEntry;
	float entry_x, entry_y, entry_l;
	int flag;
	
	double base_width;
	bool has_norots;
	
	bool fallout, panProblem; // panProblem indicates that this level draws too slowly.
	string falloutMessage;
	
	cpSpace *space;
	cpBody *staticBody;
	
	bool landed;
	bool anytilts; // true if anything's in tilt[]
	queue<string> messages;
	list<splode_info> splodes;
	list<trail_info> trails;
	hash_map<unsigned int, cpShape *> mans; // the unsigned int is also a cpshape* ARRGH STL SUCKS
	hash_map<unsigned int, cpShape *> tilt; // the unsigned int is also a cpshape* ARRGH STL SUCKS

	spaceinfo() {
		landed = false;
		for(int c = 0; c < MAXLAYERS; c++) {
			r[c] = 1; g[c] = 1; b[c] = 1; 
		}
		layers = 0;
		num = 0;
		flag = 0;
		haveEntry = false; entry_x = 0; entry_y = 0; entry_l = 0;
		camera = cam_track; rots = 0; dontrot = 0; rspeed = 1;
		space = 0; staticBody = 0;
		base_width = 0; has_norots = false;
		repeat = repeat_no; repeat_every = 0; reprot = false; reprots = 0; anytilts = false;
		for(int x = 0; x < 4; x++) {
			for(int y = 0; y < 4; y++) {
				disprot[x][y] = 0;
			}
		}
		master_tiltg = cpv(1,0);

		deep = -1; after = 1; zoom = 1;
		fallout = false; panProblem = false;
	}
	
	// This entire feature is implemented terrible but I'm too tired at this point to think about doing it right
	inline int tiltfor(int x, int y) { x %= 4; x += 4; x %= 4; y %= 4; y += 4; y %= 4; return disprot[x][y]; } 
	inline unsigned int getReprot(int i) {
		return ((reprots >> (i*4)) & REPROTALL); 
	}
	inline int reprotForDir(bool yaxis, bool positive) {
		int r = (positive?0:2) + (yaxis?0:1);
//		printf("RETURN ROT AT: %d\n", zzz); // UGGGLY
		return getReprot( r );
	}
};
//extern hash_map<cpSpace *, spaceinfo> backtrack;

inline bool okRot(void *p) { return !(p && ((enemy_info *)p)->noRot); }

struct ing_info : public enemy_info {
	int runningSince;
	ing_info(int _runningSince) : enemy_info(), runningSince(_runningSince) {}
	virtual cloneable *clone() { return new ing_info(runningSince); }
};

struct loose_info : public enemy_info {
	cpFloat m, i;
	loose_info(cpFloat _m, cpFloat _i) : enemy_info(false) { m = _m; i = _i; }
	virtual cloneable *clone() { return new loose_info(m, i); }
};

// JUMPMAN

extern int jumpman_s, jumpman_d, jumpman_l; extern float jumpman_x, jumpman_y, jumpman_r, scan_x, scan_y, scan_r;
extern cpVect rescan;
extern bool jumpman_unpinned;
enum jumpman_state {
	jumpman_normal = 0,
	jumpman_splode,
	jumpman_pan,
	jumpman_wantsplode,
	jumpman_wantexit,
};
extern jumpman_state jumpmanstate;
enum pan_type {
	pan_dont = 0,
	pan_deep,
	pan_side
};
extern pan_type pantype;
extern float panfz, panfr, pantz, pantr; extern cpVect panf, pant; extern int pantt, panct, jumptt, jumpct;
extern double aspect;
extern double surplusSpin;

// CONTROLS

#define ROTSTEPS 12
extern int wantrot;
extern int rotstep;
extern double roten;
double frot(const double &in);

#define NUMCONTROLS 7
#define NUMEDITCONTROLS 5
#define LEFT 0
#define RIGHT 1
#define JUMP 2
#define ROTR 3
#define ROTL 4
#define CONTROLS 5
#define QUIT 6
void addFloater(string newFloater);
void controlsScreen(bool startdone = false);
void populateControlsScreen(ContainerBase *intoContainer, string mention = string());
void layerYell();
void rotYell(bool atall, bool fogged);
void endingYell(const char *msg, double x, double y, int multiplier = 1);

// STATE

extern vector<spaceinfo> level;
extern vector<int> flags;

extern hash_map<unsigned int, plateinfo *> pinfo;

struct scoreinfo {
	vector<int> time;
	vector<int> deaths;
};

// I REALLY HATE THE STL
namespace __gnu_cxx                                                                                 
{                                                                                             
  template<> struct hash< std::string >                                                       
  {                                                                                           
    size_t operator()( const std::string& x ) const                                           
    {                                                                                         
      return hash< const char* >()( x.c_str() );                                              
    }                                                                                         
  };                                                                                          
}          

extern hash_map<string, pair<scoreinfo, scoreinfo> > scores;
extern int jumpman_flag;
extern string currentScoreKey;
void timer_flag_rollover();

inline void ensurecapacity(vector<int> &s, int capacity) {
	if (s.size() < capacity)
		s.resize(capacity);
}
inline void ensurecapacity(pair<scoreinfo, scoreinfo> &s, int capacity) {
	ensurecapacity(s.first.time, capacity);
	ensurecapacity(s.first.deaths, capacity);
	ensurecapacity(s.second.time, capacity);
	ensurecapacity(s.second.deaths, capacity);
}

// PROTOTYPES

bool BackOut();
void Quit(int code = 0);
void BombBox(string why = string());
void FileBombBox(string filename = string());

extern cpBody *chassis;
extern cpShape *chassisShape;

extern void moonBuggy_init(void);
extern void moonBuggy_input(int button, int state, int x, int y);
extern void moonBuggy_update(void);
extern void moonBuggy_keyup(unsigned char key);
extern void moonBuggy_keydown(unsigned char key);
extern void clickLastResortHandle(double x, double y, int button, bool justDragging = false);
extern void loadGame(const char *filename);

extern cpFloat input_power;
extern cpFloat input_power_last_facing;
extern cpFloat input_power_modifier;
extern bool jumping;
extern int started_moving_at_ticks;
extern int anglingSince;

extern TiXmlDocument *editing;
extern string editingPath;
extern TiXmlNode *editLevel;
extern vector<slice *> editSlice;
extern vector<string> editSlicePath;
extern int editLayer;

extern bool rePerspective;

extern int ticks;

extern bool doInvincible, doingInvincible;
extern int lastRebornAt;
inline bool invincible() { return doingInvincible && ticks-lastRebornAt < 60; }

void loadLevel(spaceinfo *s, slice &levelpng, int r, unsigned int l = 0);
void loadLevel(TiXmlNode *levelxml, const char *pathname, bool noFiles = false);
void jumpman_reset (bool needsadd = true);
void justLoadedCleanup();
string nameFromLevel(TiXmlNode *_element);
void clearEverything();
string srcFromLevel(TiXmlNode *_element);
string dotJmp(string name);

void loadEditorFile(const char *filename);
extern cpSpace *uiSpace;

extern bool haveTimerData, wantTimerData, censorTimerData;
extern int timerStartsAt, timerEndsAt, timerLives, subTimerLives, timerPausedAt;
void startTimer();
void completeTimer();
void pause(bool _paused);

// interface
extern ContainerBase *columns[3];
void serviceInterface();
extern bool wantClearUi;
enum WantNewScreen {
	WNothing = 0,
	WReset,
	WMainMenuPaused,
	WMainMenu,
	WEditMenu,
	WDisplayOptions,
	WNewFileEntry,
	WCurrentFileDirectory,
	WPlay,
	WSaveThenLevelMenu,
	WLevelMenu,
	WPalette,
	WMorePalette,
	WSave,
	WSetup,
	WColor,
	WLayers,
	WAngle,
	WLevelDupe,
	WLevelDelete,
	WSaveAngleThenLevelMenu,
	WUpDown,
	WPause,
	WUnpause,
	WEndingExit,
	WResumeCredits,
	WIgnore
};
enum EditMode {
	ENothing = 0,
	EPlayground,
	EWalls,
	EAngle,
	ECoordinates
};
extern WantNewScreen want, onEsc, onWin; 
extern bool paused, drawPalette, drawControls, completelyHalted; // The three out of that group of four used outside main.cpp
extern bool haveLineStart, dragPoison; extern int lineStartX, lineStartY;
extern bool editorPanning;
extern EditMode edit_mode; 
void repaintEditSlice();
void reentryEdit();
extern unsigned int chosenTool;
void dummyStage();

extern bool haveWonGame;

void BlindColorTransform(double &, double &, double &, unsigned int);

void SaveHighScores();

extern list<string> toolHelp;

extern double lastpanfadez;
extern int exit_direction;

extern bool optSplatter, &optColorblind, &optAngle, &optSlow, &optWindow;
void initFog();
extern int desiredEnding;
void startEnding();
void endEnding();

inline int iabs(int i) { return i < 0 ? -i : i; }
inline int howDeepVisible() {
	if (optSlow)
		return 2;
	return 4;
}
inline bool okayToDraw(int deep) {
	if (completelyHalted) return false;
	
	int gap = howDeepVisible();
	if (pantype != pan_deep)
		return deep < jumpman_d && deep >= jumpman_d - gap;
	else {
		double _deep = deep;
		double camera = -lastpanfadez;
//		fprintf(stderr, "DEEP? %lf - %lf = %lf\n", _deep, camera, _deep - camera);

		_deep -= camera;
		return _deep <= -0.5 && _deep > (-1.0-gap);
	}
}
inline int tiltrightnow() { return level[jumpman_s].tiltfor(floor(rescan.x/level[jumpman_s].repeat_every+0.5), floor(rescan.y/level[jumpman_s].repeat_every+0.5)); }
inline float rFor(spaceinfo &s) { return (s.zoom > 0 && edit_mode != EWalls ? s.zoom : 1.0) * (cam_fixed == s.camera ? 1.0 : 3.0/2); }

void externSpaceRemoveBody(spaceinfo &s, cpShape *shape);

#if 1 && SELF_EDIT
#define ERR(...) fprintf (stderr, __VA_ARGS__)
#else
#define ERR(...)
#endif
#define REALERR(...) fprintf (stderr, __VA_ARGS__)