/*
 *  jumpman.h
 *  Jumpman
 *
 *  Created by Andi McClure on 4/27/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "slice.h"
#include "sound.h"
#include "tinyxml.h"
#include "internalfile.h"
#include "condition.h"
#include <math.h>
#include <list>
#include <queue>

// FIXME
#ifndef GLfloat
#define GLfloat float
#endif

// CONSTANTS

#define NORMALANGLE 1

// These correspond, perhaps foolishly, to colors.
#define C_JUMPMAN  9000ul
#define C_RESIZE   9100ul
#define C_RESIZE2  9110ul
#define C_LINE     9120ul
#define C_SCROLL   9130ul
#define C_ERASER   9140ul
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
#define C_BOMBTYPE(x) ((x) == C_BOMB || (x) == C_BOMB2)

// TODO: Speed should be dynamic?

#define VSYNC_DESPERATION 0
#define FPS 80.0
#define TPF ((int)(1000/FPS))
#define REAL_FPS 60.0

#define OKTHREAD 1
#define DBGTHREAD   0
#define DBGTIME     0
#define FOG_DEBUG 0 // SET BOTH

#define MAXLAYERS 32

#define COMPILED_VERSION 1.1

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

enum LoadedFrom {
    FromInternal = 1,
    FromOwn,
    FromDown
};

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

struct coord {
	float x1, y1, x2, y2;
	coord() { x1 = y1 = x2 = y2 = 0; }
	coord(float _x1, float _y1, float _x2, float _y2, int sq = 128) {
		x1 = _x1/sq;
		y1 = _y1/sq;
		x2 = _x2/sq;
		y2 = _y2/sq;
	}
};

// TODO: Move to another file, avoid warnings
static const cpVect def_verts[] =   { cpv(-19.2,-19.2), cpv(-19.2, 19.2), cpv( 19.2, 19.2), cpv( 19.2,-19.2), };
static const cpVect ball_verts[] =  { cpv(-19.2*2,-19.2*2), cpv(-19.2*2, 19.2*2), cpv( 19.2*2, 19.2*2), cpv( 19.2*2,-19.2*2), };
static const cpVect jump_verts[] =  { cpv(-14.4,-19.2), cpv(-14.4, 19.2), cpv( 14.4, 19.2), cpv( 14.4,-19.2), };
static const cpVect mini_verts[] =  { cpv(-19.2/2,-19.2/2), cpv(-19.2/2, 19.2/2), cpv( 19.2/2, 19.2/2), cpv( 19.2/2,-19.2/2), };

static unsigned int packColor(float r, float g, float b, float a = 1) {
	unsigned int color = 0;
	unsigned char c;
	c = a*255; color |= c; color <<= 8;
	c = b*255; color |= c; color <<= 8;
	c = g*255; color |= c; color <<= 8;
	c = r*255; color |= c;
	return color;
}

struct plateinfo {
	int width, height;
	int frames;
	float r, g, b;
	unsigned int color;
	cpVect constv; float u;
	platebehave behave;
	slice **slices;
	GLfloat **arrays;
	int *quadCount;
	unsigned int *texture;
	coord *coords;
	const cpVect *drawShape;
	cloneable *defaultdata;
	plateinfo() {
		width = 0; height = 0; frames = 0; arrays = 0; quadCount = 0; texture = 0; color=0;
		r = 1; g = 1; b = 1;
		constv = cpvzero; u = 0.5;
		behave = nobehave;
		slices = NULL;
		coords = NULL;
		defaultdata = NULL;
		drawShape = def_verts;
	}
	~plateinfo() {
		if (arrays) {
			for(int c = 0; c < frames; c++)
				delete[] arrays[c];
			delete[] arrays;
		}
		if (quadCount)
			delete[] quadCount;
		if (texture)
			delete[] texture;
		if (coords)
			delete[] coords;
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
	float r, g, b;
	cpVect p;
	bool reflect;
	int ct, tt;
    float rot_pre, rot_post;
	splode_info(spaceinfo *space, cpShape *shape, int frame, cpVect _p, bool _reflect);
	void adjust_sound();
};

struct trail_info {
	cpVect at, trail;
	float r, g, b;
	int ct, tt;
	trail_info(cpVect _at, cpVect _trail, float _r, float _g, float _b);
};

enum trace_type {
	t_null = 0,
	t_move,
	t_swipe,
	t_nullmove,
	t_jump,
	t_rot
};
struct trace_info {
	cpVect at;
	int ct;
    float r, g, b;
    float rad;
};

struct enemy_info : public cloneable {
	int lastWhichFrame;
	bool lastReflect;
	bool noRot;
	int tiltid; // or -1 for mans
	bool tiltref;
	cpVect tiltg;
	int megaOffset; // or -1 for "not in space"
	
	enemy_info(bool _noRot = false) { lastWhichFrame = 0; lastReflect = false; noRot = _noRot; tiltid = 0; tiltref = false; tiltg = cpv(1,0); megaOffset = -1;}
	virtual cloneable *clone() { return new enemy_info(noRot); } // Remember, C++ won't forward this in children...
	
	enemy_info(ifstream &f);
	virtual void hib(ofstream &f);
};

#define REPROTROT 3
#define REPROTREF 4
#define REPROTALL 7

#define MEGA_STRIDE 5
#define STATIC_STRIDE 3
#define LAVA_STRIDE 2

struct quadpile : public vector<float> {
	static vector<unsigned short> megaIndex;
	static void megaIndexEnsure(int count);
	void push(float x1, float y1, float x2, float y2, float r, float g, float b, float a = 1.0, float *texes = NULL);
};

struct spaceinfo {
	float r[MAXLAYERS], g[MAXLAYERS], b[MAXLAYERS];
	int num;
	int deep;
	string name;
	
	int layers;
	camera_type camera;
	repeat_type repeat;
	
	bool reprot;	// Do we rotate on repeat?
	unsigned int reprots; // How do we rotate on repeat?
	char disprot[4][4];
	cpVect master_tiltg;
	
	float repeat_every;
	unsigned char rots, orots; // It's a bitmask! I am so sorry!
	unsigned char dontrot, odontrot; // It's also a bitmask!
	int rspeed;
	float zoom, special_zoom; unsigned int special_mask;
	int after; // Will be subtracted from the next "deep".
	
	bool haveEntry;
	float entry_x, entry_y, entry_l;
    bool haveExit; // ONLY USED WHEN "SKIP" IN PLAY
    float exit_x, exit_y;
	int flag;
	
	float base_width;
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
	
	quadpile staticVertex;	// Floors
	quadpile lavaVertex;	// Lava
	quadpile zoneVertex;		// norot zones
	quadpile kludgeVertex; // Used for: static, non-floor objects
	quadpile megaVertex;	// Active objects
	
    bool loaded;
    deque<TiXmlElement *> files; // Used in loading
    
	spaceinfo() {
		landed = false;
		for(int c = 0; c < MAXLAYERS; c++) {
			r[c] = 1; g[c] = 1; b[c] = 1; 
		}
		layers = 0;
		num = 0;
		flag = 0;
		haveEntry = false; entry_x = 0; entry_y = 0; entry_l = 0;
        haveExit = false; exit_x = 0; exit_y = 0;
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

		deep = -1; after = 1; zoom = 1; special_zoom = 1; special_mask = 0;
		fallout = false; panProblem = false;
        loaded = false;
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
    void realForceLoad();
    void forceLoad();
    inline void tryLoad() {
        if (!loaded)
            forceLoad();
    }
};
//extern hash_map<cpSpace *, spaceinfo> backtrack;

inline bool okRot(void *p) { return !(p && ((enemy_info *)p)->noRot); }

struct ing_info : public enemy_info {
	int runningSince;
	ing_info(int _runningSince) : enemy_info(), runningSince(_runningSince) {}
	virtual cloneable *clone() { return new ing_info(runningSince); }
	
	ing_info(ifstream &f);
	virtual void hib(ofstream &f);
};

struct loose_info : public enemy_info {
	cpFloat m, i;
	loose_info(cpFloat _m, cpFloat _i) : enemy_info(false) { m = _m; i = _i; }
	virtual cloneable *clone() { return new loose_info(m, i); }
	
	loose_info(ifstream &f);
	virtual void hib(ofstream &f);
};

// JUMPMAN

extern int jumpman_s, jumpman_d, jumpman_l; extern float jumpman_x, jumpman_y, jumpman_r, scan_r;
extern cpVect scan;
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
extern float aspect;
extern float surplusSpin;
extern bool exit_grav_adjust;
extern int rotting_s, want_rotting_s;
extern float grav_adjust_stashed_roten;

// CONTROLS

#define ROTSTEPS 12
extern int wantrot;
extern int rotstep;
extern float roten;
float frot(const float &in);

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
void layerYell();
void rotYell(bool atall, bool fogged, bool dir);
void endingYell(const char *msg, float x, float y, int multiplier = 1);

// STATE

extern vector<spaceinfo> level;
extern vector<int> flags;

extern hash_map<unsigned int, plateinfo *> pinfo;

struct scoreinfo {
	vector<int> time; // Note: time == 0 means "i cheated"
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
extern hash_map<string, int> skips;
extern hash_map<string, cpVect> skips_entry;
extern int jumpman_flag;
extern string currentScoreKey;
extern int currentSkip, willSkipNext;
extern float currentSkipX, currentSkipY, willSkipNextX, willSkipNextY;
extern bool skipOnRebirth, packBansSkip, exitedBySkipping, flagIsSkipPoisoned;
void timer_flag_rollover();

extern bool readyScores, dirtyScores;
void SaveHighScores(); // At its own option, may do nothing.
void LoadHighScores();

void scoreKeyIs(LoadedFrom from, string filename);
void clearScoreKey(LoadedFrom from, string filename);

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
extern void clickLastResortHandle(cpVect click, int button, bool justDragging, cpVect &originat);
extern void wload(LoadedFrom from, string filename, bool buildScoreKey = false, bool andClearEverything = true);
extern void loadGame(LoadedFrom from, const char *filename, int start_at_flag);

extern cpFloat input_power;
extern cpFloat input_power_last_facing;
extern cpFloat input_power_modifier;
extern bool jumping;
extern int started_moving_at_ticks;
extern int anglingSince;

extern TiXmlDocument *editing;
extern string editingPath, editBaseName;
extern TiXmlNode *editLevel;
extern vector<slice *> editSlice, editSliceOld;
extern vector<string> editSlicePath;
extern int editLayer;
void wipe(vector<slice *> &v);

extern bool dirtyLevel;
extern vector<bool>dirtySlice;
void cleanSlices();
void cleanAll();

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
extern uint64_t timerStartsAt, timerEndsAt, timerPausedAt; extern int timerLives, subTimerLives;
void startTimer();
void completeTimer();
void pause(bool _paused);

// interface
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
    EZoom,
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

#define DOES_TOUCHES(x) ((x) == EWalls || (x) == EAngle || (x) == EPlayground)

extern bool haveWonGame;

void BlindColorTransform(float &, float &, float &, unsigned int);

extern list<string> toolHelp;

extern float lastpanfadez;
extern int exit_direction;

extern bool optSplatter, &optColorblind, &optAngle, &optSlow, &optWindow;
extern bool textureMode, optAxis, drawGrid, drawEsc;
extern bool haveWonGame, justWonGame;
void initFog();
extern int desiredEnding;
void startEnding();
void endEnding();

inline int iabs(int i) { return i < 0 ? -i : i; }
inline int howDeepVisible() {
	return 3;
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
		float _deep = deep;
		float camera = -lastpanfadez;
//		fprintf(stderr, "DEEP? %f - %f = %f\n", _deep, camera, _deep - camera);

		_deep -= camera;
		return _deep <= -0.5 && _deep > (-1.0-gap);
	}
}
inline int tiltrightnow() { return level[jumpman_s].tiltfor(floor(rescan.x/level[jumpman_s].repeat_every+0.5), floor(rescan.y/level[jumpman_s].repeat_every+0.5)); }

void externSpaceRemoveBody(spaceinfo &s, cpShape *shape);
void externSpaceAddBody(spaceinfo &s, cpShape *shape);

extern float accX, accY, accZ;
void newarrows(bool l, bool r);
void jump();
bool rotkey(char key); // True if rotated

enum ControlModeMove {
    CMoveDisabled = 0,
	CPush = 1,
	CSwipe = 2,
	CButtonMove = 4,
	CMoveAuto = 8,
    CFollow = 16,
    
    CEditMove = 32,
};

enum ControlModeRot {
    CRotDisabled = 0,
    CGrav = 1,
	CTilt = 2,
	CKnob = 4,
	CButtonRot = 8,
    
	CEditRot = 16,
};

#define MODE_JUMPS(x) ((x) != CButtonMove && (x) != CFollow)

extern ControlModeMove controlMove, savedControlMove;
extern ControlModeRot controlRot, savedControlRot;
extern bool dirtySettings;

void setControlMove(unsigned int _move, bool permanent = true);
void setControlRot(unsigned int _rot, bool permanent = true);

inline float rFor(spaceinfo &s) { 
    return (s.zoom > 0 && edit_mode != EWalls ? s.zoom * (savedControlRot & s.special_mask ? s.special_zoom : 1.0) : 1.0) 
    * (cam_fixed == s.camera ? 1.0 : 3.0/2);
}

extern LoadedFrom lastLoadedFrom;
extern string lastLoadedFilename, lastLoadedAuthor;
extern string currentlyLoadedPath;
extern bool bombExcept;
void deepEditLoad( TiXmlNode * target );
extern int availableLevelsCount;
extern bool availableLevelsAreFake;

extern int blockload_s; // "block" as in "prevent"

// CONVERSIONS

struct CGPoint;
cpVect screenToView(CGPoint in);
cpVect viewToSpace(cpVect in);
cpVect spaceToView(cpVect in);

extern int holdrot;
void this_side_up();
void wantEsc(bool w);
void GoWinScreen();

void SaveHibernate();
bool LoadHibernate();
void LoseHibernate();
extern bool didHibernate, wantHibernate;

extern float overflowv;

extern int tiltFlare[2];

struct averager {
    queue<float> window;
    float accum;
    int maxcount;
    averager(int _count);
    void push(float v);
    float at();
};

struct tilter {
    char key; // Rotation result

    virtual bool enabled() { return true; }
    
    virtual float rawAt() { return 0; }
    virtual float center() { return 0; }
    virtual float at() { return 0; }
    virtual float line(bool high) { return 0; }
    virtual float safe(bool high) { return 0; }
    virtual float shadow(bool high) { return 0; }
    
    virtual void run() {}
    virtual void reset() {}
};

extern tilter *tilt;

void adjustControlRot();

void userPath(char *dst);
#define DIRPATH "own"
#define DOWNPATH "down"

#if DBGTHREAD
#define THREADERR ERR
#else
#define THREADERR(...)
#endif

#if SELF_EDIT
#define ERR(...) printf (__VA_ARGS__)

#define GLERR(x) \
for ( GLenum Error = glGetError( ); ( GL_NO_ERROR != Error ); Error = glGetError( ) )\
printf("GLERR %s: %u\n", x, Error);

#else
#define ERR(...)
#define GLERR(...)
#endif
#define REALERR(...) printf (__VA_ARGS__)

#if OKTHREAD
extern condition needed;   // work needed
extern condition completed;     // work done
void halt_worker();
extern bool worker_alive; // Only touchable by GUI thread.
extern bool need_halt; // Set when need to put worker to sleep, clear when done.
extern bool need_detach; // Set when need to kill worker, clear when done.
extern bool need_load; // Set when need worker to read xml, clear when done.
extern bool need_some_load; // Set when worker to load some level, clear when done.
extern int some_load_prioritize; // Set when worker to load some level; worker sets to 0 after reading
#endif

void wantMusic(bool music);

inline void rotl(unsigned char &i) {
	i = (i << 1) | (i & 0x80 ? 1 : 0);
}

inline void rotr(unsigned char &i) {
	i = (i >> 1) | (i & 0x01 ? 0x80 : 0);
}

