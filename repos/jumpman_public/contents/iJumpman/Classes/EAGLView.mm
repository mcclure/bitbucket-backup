//
//  EAGLView.m
//  iJumpman
//
//  Created by mcc on 3/7/09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "EAGLView.h"
#import "GLViewController.h"
#include "mutils.h"

#include "iJumpman.h"
#include "color.h"

#include <sys/stat.h>
#include <queue>
#include <stdexcept>

#include <AVFoundation/AVFoundation.h>
#include "Texture2D.h"

#include "glCommon.h"
#include "glCommonMatrix.h"

#include "TinyLang.h"

#define USE_DEPTH_BUFFER 0
#define DRAW_DEBUG 0
#define GYRO_DEBUG 0
#define GESTURE_DEBUG 0
#define TAP_DEBUG 0
#define TEXTURE_DRAW 1
#define TEXTURE_SQUARE 1
#define FLICKER_DEBUG 0
#define TEXTURE_MODE_VISIBLE 0
#define OKAY_SHUFFLE 1
#define OKAY_TEXT 1
#define OKAY_TRACER 1
#define FTMAX 64
#define NEW_VELOCITY 1

// Originally this was meant to be on all the time.
#define FB_DRAW 0
#define FB_SIZE 256

#define DEADZONE (1.0/3/4)

#define SWIPESWITCH_AT 10
#define PUSHSWITCH_AFTER 10

#define RAD(x) ((x)/180.0*M_PI)

#define FPS34 (FPS*3.0/4)

#if OKAY_SHUFFLE
#include <MediaPlayer/MediaPlayer.h>
#endif

// ------------- C TYPE STUFF -- POSSIBLY TEMPORARY --------------------

UIView *globalWindow = nil;
BOOL transitioning = NO;

hash_map<string, coord> atlascoords;
coord arrowcoords[4];
coord norotcoord;
GLuint atlas, arrow_atlas, mountain = 0;
LoadedFrom lastLoadedFrom;
string lastLoadedFilename, currentlyLoadedPath, lastLoadedAuthor;
bool bombExcept = false;
bool fastAsleep = false; // TODO: Not used for anything yet

GLfloat edit_model_view[16];
GLfloat edit_projection[16];
GLint edit_viewport[4];	
void calculateCoordinates();

ControlModeMove controlMove = CMoveDisabled, savedControlMove = (ControlModeMove)(CPush | CMoveAuto);
ControlModeRot controlRot = CRotDisabled, savedControlRot = (ControlModeRot)(CGrav);
const int controlButtonWidth = 107;
int buttonFlare[6] = {0,0,0,0,0,0};
int tiltFlare[2] = {0,0};
bool dirtySettings = false;

bool optAxis = GYRO_DEBUG, textureMode = true, drawGrid = true, drawEsc = false;
bool haveWonGame = false, justWonGame = false;

int lastdrawescat = 0;
cpVect escAt = cpvzero; bool escHave = false; unsigned int escSince = 0; // drawEsc for "we want an esc" escHave for "we've successfully drawn an esc"

GLuint normalFb = 0, normalWidth = 0, normalHeight = 0; // Can be removed?

int availableLevelsCount = 1;
bool availableLevelsAreFake = false;

// Music state
@interface MusicDelegate : NSObject <AVAudioPlayerDelegate> {
}
- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag;
- (void)audioPlayerDecodeErrorDidOccur:(AVAudioPlayer *)player error:(NSError *)error;
@end
AudioMode audioMode = LOCAL_AUDIO;
MusicDelegate *musicDelegate = nil;
void killMusic();
void kickMusic();
void AudioHalt();
void AudioResume();

// Intro state
bool intro = true;
void display_intro();
bool basicInit = false;

string haltMessage;
bool loadedHibernate = false;

const GLfloat scale = 2.4;

int rotYellingSince = 0, rotYellingIconSince = 0, rotYellingIconRot = 0; bool rotYellingIconDir = false;
bool restrictDraw = false, restrictDrawObject = false;
unsigned int restrictDrawTo = 0, restrictDrawToInt = 0;
bool drawingNoRots = false;

float stencilr = 0, stencilg = 0, stencilb = 0, stencila = 0;

GLint viewport[4] = {0,0,0,0};

#define TRACELIFE 40
CGSize ztracebounds_orig = {0,0}, ztracebounds = {0,0};

trace_info screentraces[FTMAX]; // Who needs that stupid STL anyway.
unsigned int screentraces_base = 0, screentraces_count = 0;
inline void screentraces_pop_back() { screentraces_count--; screentraces_base = (screentraces_base+1)%FTMAX; }
inline trace_info &screentraces_front() {
    return screentraces[(screentraces_base+screentraces_count-1)%FTMAX];
}
inline trace_info &screentraces_back() {
    return screentraces[screentraces_base];
}
inline void screentraces_push_front(cpVect _at, trace_type _type, bool _huge = false) {
    if (screentraces_count < FTMAX) {
        screentraces_count++;
        trace_info &i = screentraces_front();
        i.at = _at; i.ct = 0; i.rad = 1;
        switch(_type) {
            case t_null:
                i.r = 0.5; i.g = 0.5; i.b = 0.5;      // Color Gray?
                break;
            case t_move:
                i.r = 1.0; i.g = 0.0; i.b = 0.0;      // Color Red?
                break;
            case t_swipe:
                i.r = 1.0; i.g = 0.0; i.b = 1.0; // Color Purple?
                break;
            case t_nullmove:
                i.r = 0.33333; i.g = 0.33333; i.b = 0.33333;      // Color Darkred?
                break;
            case t_jump:
                i.r = 0.0; i.g = 1.0; i.b = 1.0;      // Color Cyan?
                i.rad *= 2;
                break;
            case t_rot:
                i.r = 0.0; i.g = 1.0; i.b = 0.0;      // Color Green?
                i.rad *= 2;
                break;
        }        
        if (0&&_huge)
            i.rad *= 3;
    }
}
inline void screentraces_clear() {
    screentraces_base = 0; screentraces_count = 0;
}


#if !TEXTURE_DRAW
// Is this stupid?
// perl -e '$a = 30; $pi = 3.14159265358979323846; for(0..$a) {$b = 2*$pi*$_/$a; $x = cos($b); $y = sin($b); print "$x,$y,"; } print "\n"'
#define CIRCLELEN 62
GLfloat traceCircle[CIRCLELEN] = {
1,0,0.978147600733806,0.207911690817759,0.913545457642601,0.4067366430758,0.809016994374947,0.587785252292473,0.669130606358858,0.743144825477394,0.5,0.866025403784439,0.309016994374947,0.951056516295154,0.104528463267653,0.994521895368273,-0.104528463267653,0.994521895368273,-0.309016994374947,0.951056516295154,-0.5,0.866025403784439,-0.669130606358858,0.743144825477394,-0.809016994374947,0.587785252292473,-0.913545457642601,0.4067366430758,-0.978147600733806,0.207911690817759,-1,5.66553889764798e-16,-0.978147600733806,-0.207911690817759,-0.913545457642601,-0.4067366430758,-0.809016994374947,-0.587785252292473,-0.669130606358858,-0.743144825477394,-0.5,-0.866025403784438,-0.309016994374948,-0.951056516295154,-0.104528463267654,-0.994521895368273,0.104528463267653,-0.994521895368273,0.309016994374947,-0.951056516295154,0.5,-0.866025403784439,0.669130606358858,-0.743144825477394,0.809016994374947,-0.587785252292473,0.913545457642601,-0.4067366430758,0.978147600733806,-0.207911690817759,1,0
};
#endif
GLfloat traceLine[4] = {
-1,0,1,0
};
quadpile traceBox, traceBoxUp;

const GLfloat textureEdges[] = {
-1.0, 1.0,     // Top left
-1.0, -1.0,    // Bottom left
1.0, -1.0,     // Bottom right
1.0, 1.0       // Top right
};

unsigned int lastWraithAt = 0;
UITouch *lastJumpAt = NULL;
bool nextMoveHuge = false;

#if DBGTIME
#include <sys/time.h>
int lsec = 0; int lusec = 0;
void printTimeOfDay(const char *who = "") {
    timeval tp;
    gettimeofday(&tp, NULL);
    ERR("\tTIME%s\t%d\t -- \t%d\t ||| \t%d\t -- \t%d\n", who, (int)tp.tv_sec, (int)tp.tv_usec, (int)tp.tv_sec-lsec, (int)tp.tv_usec-lusec);
    lsec = tp.tv_sec; lusec = tp.tv_usec;
}
#else
#define printTimeOfDay(x) 
#endif

inline void switchauto(ControlModeMove to) {
	controlMove = (ControlModeMove)(CMoveAuto | to);
    nextMoveHuge = true;
}
bool autostill = false;
int autostillsince = 0;


// TEXT
enum lockoutTypes {
	lockoutNone = 0,
	lockoutHeavy,
	lockoutLayer,
	lockoutRot,
	lockoutDebug
};

struct floater {
	int t, c;
    int born; // tick count -- used by ending
	float x, y, dx, r, g, b;
	lockoutTypes lockout;
	string text;
    Texture2D* texture;
    float stickyRot; bool haveStickyRot;
    
	floater(string _text = string(), int _t = 0, int _c = 0, float _x = 0, float _y = 0, float _dx = 0, lockoutTypes _lockout = lockoutNone, float _r = 1, float _g = 1, float _b = 1) {
		text = _text;
		t = _t; c = _c;
		x = _x; y = _y; dx = _dx;
		r = _r; g = _g; b = _b;
		lockout = _lockout;
        texture = NULL;
        stickyRot = 0; haveStickyRot = false;
        born = endingAt;
	}
    inline void live() {
        if (!texture)
            texture = [[Texture2D alloc] 
                                    initWithString:toNs(text.c_str())
                                    dimensions:CGSizeMake(320, 320) alignment:UITextAlignmentCenter fontName:@"Helvetica" 
                                    fontSize:15];        
    }
    inline void die() {
        if (texture) // Redundant due to nature of objc, but I don't care.
            [texture release];
    }
};

list<floater> drawingFloaters; // Some floaters (all floaters, in the iphone version?) are "heavy". Only one "heavy" can be drawn
queue<string> pendingFloaters; // at a time. Heavy strings live in pendingFloaters until they're ready to be drawn, then move to
bool floatheavy = false;        // drawingFloaters. floatheavy tracks whether at least one heavy is being drawn.
int floatcount = 0; // # heavies ever seen

float centerOff(const char *str) { return 0; }
void resetFloater() {
	if (pendingFloaters.size() > 0 && (drawingFloaters.size() == 0 || !floatheavy)) {
		int side = floatcount % 2?-1:1;
		drawingFloaters.push_back( floater(
                                           pendingFloaters.front(),
                                           FPS*4 * 3/4.0, // Cuz framerates are all wrong in iphone version
                                           0,
                                           300*side - centerOff(pendingFloaters.front().c_str()),
                                           -200*side,
                                           0.25*-side,
                                           lockoutHeavy) );
		pendingFloaters.pop();
		floatcount++;
		floatheavy = true; // Block out any other heavies
	}
}
void addFloater(string newFloater) { // Adds "heavy" (exclusive) floaters only
#if OKAY_TEXT
	pendingFloaters.push(newFloater);
	resetFloater(); // Will do nothing if there's already a heavy floater
#endif
}
void clearFloaters() {
    for(list<floater>::iterator b = drawingFloaters.begin(); b != drawingFloaters.end(); b++) {
        (*b).die();
    }
    drawingFloaters.clear();
    while (!pendingFloaters.empty())
        pendingFloaters.pop();
}

void endingYell(const char *msg, float x, float y, int multiplier) {
	x *= 750.0/2;
	y *= 750.0/2;
	drawingFloaters.push_back( floater(
                                       msg,
                                       FPS*4*multiplier * 3/4.0,
                                       0,
                                       x - centerOff(msg)/2,
                                       y /*- floatHeight/2*/,
                                       -0.25,
                                       lockoutNone,
                                       0,
                                       0,
                                       0) );
    
}

bool haveEditorYelled = false;
void editorYell(const char *msg, float r, float g, float b) {
    if (haveEditorYelled) return;
    haveEditorYelled = true;
    float x = 0;
    float y = 0;
    drawingFloaters.push_back( floater(
                                       msg,
                                       FPS*4*2 * 3/4.0,
                                       0,
                                       x - centerOff(msg)/2,
                                       y /*- floatHeight/2*/,
                                       -0.25,
                                       lockoutNone,
                                       r,
                                       g,
                                       b) );
}

// END TEXT

cpVect screenToView(cpVect in) {
    if (controlRot & CGrav) {
        cpVect r = in;
        const cpVect center = cpv(ztracebounds.width/2, ztracebounds.height/2);
        r = cpvsub(r, center);
        r = cpvunrotate(r, cpvforangle( (tilt->center()+1)*M_PI )); // WHY +1
        r = cpvadd(r, center);
        return r;        
    }

	switch(holdrot) { // This coordinate system is a disaster, yet I'm ... duplicating it?
		case 0: default: return cpv( in.x, in.y );
		case 1: return cpv( ztracebounds_orig.height-in.y-1, in.x );
		case 2: return cpv( ztracebounds_orig.width-in.x-1, ztracebounds_orig.height-in.y-1 );
		case 3: return cpv( in.y, ztracebounds_orig.width-in.x-1 );
	}
}

cpVect screenToViewV(cpVect in) {
    if (controlRot & CGrav) {
        cpVect r = in;
        const cpVect center = cpv(ztracebounds.width/2, ztracebounds.height/2);
        r = cpvunrotate(r, cpvforangle( (tilt->center()+1)*M_PI )); // WHY +1
        return r;        
    }
    
	switch(holdrot) {
		case 0: default: return cpv( in.x, in.y );
		case 1: return cpv( -in.y, in.x );
		case 2: return cpv( -in.x, -in.y );
		case 3: return cpv( in.y, -in.x ); // I had to switch 1 and 3 to make this work. Why?
	}
}

// TRIANGLE_STRIP construction
inline void quadWrite(GLfloat *vect, const GLfloat x1, const GLfloat y1, const GLfloat x2, const GLfloat y2) {
	vect[0] = x1; vect[1] = y1;
	vect[2] = x2; vect[3] = y1;
	vect[4] = x1; vect[5] = y2;
	vect[6] = x2; vect[7] = y2;
}

inline void quadWrite2(GLfloat *vect, const GLfloat x1, const GLfloat y1, const GLfloat x2, const GLfloat y2) {
	vect[0] = x1; vect[1] = y1;
	vect[2] = x1; vect[3] = y2;
	vect[4] = x2; vect[5] = y1;
	vect[6] = x2; vect[7] = y2;
}

inline void straightWrite(GLfloat *vect, const GLfloat x1, const GLfloat y1, const GLfloat x2, const GLfloat y2) {
	vect[0] = x1; vect[1] = y1;
	vect[2] = x1; vect[3] = y2;
	vect[4] = x2; vect[5] = y2;
	vect[6] = x2; vect[7] = y1;
}

inline void straightWrite2(GLfloat *vect, const GLfloat x1, const GLfloat y1, const GLfloat x2, const GLfloat y2) {
	vect[0] = x1; vect[1] = y1;
	vect[2] = x2; vect[3] = y1;
	vect[4] = x2; vect[5] = y2;
	vect[6] = x1; vect[7] = y2;
}

inline void quadWrite(GLfloat *vect, const block &b) {
	quadWrite(vect, b.x*scale, b.y*scale, (b.x + b.width)*scale, (b.y + b.height)*scale);
}

// TRIANGLES construction
inline void quadsWrite(GLfloat *vect, const GLfloat x1, const GLfloat y1, const GLfloat x2, const GLfloat y2) {
	vect[0] = x1; vect[1] = y1;
	vect[2] = x1; vect[3] = y2;
	vect[4] = x2; vect[5] = y1;
	vect[6] = x2; vect[7] = y2;
	vect[8] = x2; vect[9] = y1;
	vect[10] = x1; vect[11] = y2;
}

// Becuase this is only used in PNG construction, at present we flip stuff.
inline void quadsWrite(GLfloat *vect, const block &b, GLfloat xoff, GLfloat yoff) {
	quadsWrite(vect, -(b.x+xoff)*scale, -(b.y+yoff)*scale, -(b.x + b.width + xoff)*scale, -(b.y + b.height + yoff)*scale);
}

void rotYell(bool atall, bool fogged, bool dir)  { 
    if (fogged)	
        rotYellingSince = ticks;
    else if (controlRot != CButtonRot && !exit_grav_adjust) {
        rotYellingIconSince = ticks;
        rotYellingIconDir = dir;
        rotYellingIconRot = lastdrawescat;
    }
}

GLuint load_texture(const char *int_name, bool interpolate = false) {
    GLuint result;
    
    char filename[FILENAMESIZE];
	internalPath(filename, int_name);
	ERR("OK? %s\n", filename);
	slice *thing = new slice();
	thing->construct(filename, false); // Doesn't REALLY construct
	
	if (!es2) {
		int width = thing->width;
		int height = thing->height;
		int xo = 0;
		int yo = 0;
		
		GLubyte textureData[width*height];
		memset(textureData, 0, sizeof(textureData));
		for(int x = 0; x < thing->width; x++) {
			for(int y = 0; y < thing->height; y++) {
				unsigned int color = thing->pixel[thing->width-x-1][thing->height-y-1];
				GLubyte present = color & 0xFF ? 255 - (color>>8) : 0;
				const int off = ((x+xo)+(y+yo)*width);
				textureData[off] = present;
			}
		}
		
		glGenTextures(1, &result);
		
		glBindTexture(GL_TEXTURE_2D, result);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolate?GL_LINEAR:GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolate?GL_LINEAR:GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, textureData);				
	} else { // Ewwww! GL_ALPHA textures wind up black RGB in es2 mode, so in es2 mode input as full GL_RGBA:
        int width = thing->width;
        int height = thing->height;
        int xo = 0;
        int yo = 0;
        
        uint32_t textureData[width*height];
        memset(textureData, 0, sizeof(textureData));
        for(int x = 0; x < thing->width; x++) {
            for(int y = 0; y < thing->height; y++) {
                unsigned int color = thing->pixel[thing->width-x-1][thing->height-y-1];
                uint32_t present = color & 0xFF ? 255 - ((color>>8) & 0xFF) : 0;
                const int off = ((x+xo)+(y+yo)*width);
                present <<= 24; // RGBA not ABGR? Bah
                present |= 0x00FFFFFF;  // RGB bits equal to white so we can multiply
                textureData[off] = present;
            }
        }
        
        glGenTextures(1, &result);
        
        glBindTexture(GL_TEXTURE_2D, result);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolate?GL_LINEAR:GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolate?GL_LINEAR:GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);				
    }
	
	delete thing;   
    
    return result;
}

void plate_atlas() {
    atlas = load_texture("atlas.png");
    arrow_atlas = load_texture("arrows.png", true);
	        
    atlascoords["ball_sad2 2.png"] = coord(0,0,32,32);
    atlascoords["ball_happy2 2.png"] = coord(33,0,65,32);
    atlascoords["ball2.png"] = coord(66,0,98,32);
    atlascoords["m 3.png"] = coord(99,0,116,17);
    atlascoords["arrow.png"] = coord(117,0,125,8);
    atlascoords["m 2.png"] = coord(99,18,116,35);
    atlascoords["m 1.png"] = coord(0,33,17,50);
    atlascoords["kyou_swoopy 1.png"] = coord(18,33,34,49);
    atlascoords["kyou_sticky 2.png"] = coord(35,33,51,49);
    atlascoords["kyou_sticky 1.png"] = coord(52,33,68,49);
    atlascoords["kyou_spiny 2.png"] = coord(69,33,85,49);
    atlascoords["jumpman1 2.png"] = coord(86,33,98,49);
    atlascoords["kyou_spiny 1.png"] = coord(99,36,115,52);
    atlascoords["kyou_bomb 2.png"] = coord(18,50,34,66);
    atlascoords["kyou_bomb 1.png"] = coord(35,50,51,66);
    atlascoords["invisible_unknown.png"] = coord(52,50,68,66);
    atlascoords["invisible_entry.png"] = coord(69,50,85,66);
    atlascoords["jumpman1 1.png"] = coord(86,50,98,66);
    atlascoords["invisible.png"] = coord(0,51,16,67);
    atlascoords["eyes5_hunter 2.png"] = coord(99,53,115,69);
    atlascoords["eyes5_hunter 1.png"] = coord(17,67,33,83);
    atlascoords["exit2.png"] = coord(34,67,50,83);
    atlascoords["exit.png"] = coord(51,67,67,83);
    atlascoords["norot.png"] = coord(68,67,84,83);
    atlascoords["jumpman1 4.png"] = coord(85,67,97,83);
    atlascoords["paintbrush.png"] = coord(0,68,16,84);
    atlascoords["kyou_swoopy 2.png"] = coord(98,70,114,86);
    atlascoords["jumpman1 3.png"] = coord(115,70,127,86);
    
	atlascoords["ball_happy2 1.png"] = atlascoords["ball2.png"];
	atlascoords["ball_sad2 1.png"] = atlascoords["ball2.png"];
    norotcoord = atlascoords["norot.png"];
        
    arrowcoords[2] = coord(2,2,142,142      ,256);
    arrowcoords[1] = coord(145,0,254,109    ,256);
    arrowcoords[0] = coord(145,110,254,219  ,256);        
}

void plateinfo::reconstruct() {
	for(int c = 0; c < frames; c++) {
		slice *thing = this->slices[c]->clone();
				
		thing->construct();
		 
		this->arrays[c] = new GLfloat[thing->blocks.size()*12];
		this->quadCount[c] = thing->blocks.size();
//		jcRotatef(180,0,0,1);
//		jcTranslatef(-(thing->width*scale)/2,-(thing->height*scale)/2,0);
		int q = 0;
		for(vector<block>::iterator b = thing->blocks.begin(); b != thing->blocks.end(); b++) {
			quadsWrite(this->arrays[c] + q++*12, *b, -thing->width/2.0, -thing->height/2.0);
		}
		
		delete thing;
	}
}

void quadpile::push(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float *texverts) {
	cpVect vertices[4] =   { cpv(x1, y2), cpv(x1, y1), cpv(x2,y1), cpv(x2,y2), };
	unsigned int color = packColor(r, g, b, a);
	for(int c = 0; c < 4; c++) {
		cpVect &v = vertices[c];
		push_back(v.x);
		push_back(v.y);
		push_back(*((float *)&color));
        if (texverts) { // Do I like this? It's only ever used by display_init/traceBox.
            push_back(texverts[c*2]);
            push_back(texverts[c*2+1]);
        }
	}			
}

void wantEsc(bool w) {
	if (w != drawEsc) {
		drawEsc = w;
		if (!drawEsc)
			escHave = false;
		else 
			escSince = ticks;
	}
	
	if (w) { // Somewhat questionably, I am using "wantEsc" as a proxy for "I have unpaused".
		if (didHibernate) { // I guess this is safe so long as wantEsc stays out of Editor mode.
			LoseHibernate();
			didHibernate = false;
		}
		wantHibernate = true;
	}
    
    if (w) {
        controlMove = savedControlMove;
        controlRot = savedControlRot;
    } else {
        controlMove = CMoveDisabled;
        controlRot = CRotDisabled;
    }
    adjustControlRot();
}

void PhoneQuittingNow() {
    pause(true); // Clear out times. FIXME: Let music keep going?
    
    cleanAll(); // Editing?
    
    SaveHighScores();
    
    if (dirtySettings) { // Just save all the settings, why not
        NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
        
        [prefs setInteger: (int) savedControlMove forKey: @"savedControlMove"];
        [prefs setInteger: (int) savedControlRot  forKey: @"savedControlRot"];
        [prefs setInteger: (int) audioMode        forKey: @"audioMode"];
        [prefs setBool:          optColorblind    forKey: @"optColorblind"];
    }
    
    if (wantHibernate)
        SaveHibernate();
    wantHibernate = false;
    
#if OKTHREAD
    if (worker_alive) {
        halt_worker();
        needed.set(need_detach, true);
        completed.wait(need_detach, false);
    }
#endif
    
    ERR("QUIT\n");
}

void PhonePausingNow() {
    if (!drawEsc) {	// Some kind of menu is open
        ARR("SLEEPLESS\n");
        return;
    }
        
    pause(true);
    wantEsc(false);
    ARR("SLEEP\n");
        
    unsigned int safeldaa = lastdrawescat; safeldaa %= 4; // Super paranoia
    NSString *directions[4] = {kCATransitionFromBottom, kCATransitionFromLeft, kCATransitionFromTop, kCATransitionFromRight};
    
#if FOG_DEBUG
    escScreen = DEBUG_SCREEN;
#endif
    
    switchScreen(escScreen, kCATransitionPush, directions[safeldaa], 0.75);		
}

void GoWinScreen() {
    switchScreen(winScreen, kCATransitionPush, kCATransitionFromBottom, 0.0);
}

void texfill(float *texout, coord co, int a, int b, int c , int d) {
    const float texes[4] = {co.x1, co.y1, co.x2, co.y2};
    const float texverts[8] = {texes[0],texes[3], texes[0],texes[1], texes[2],texes[1], texes[2],texes[3]}; 
    texout[0] = texverts[a*2]; texout[1] = texverts[a*2+1];
    texout[2] = texverts[b*2]; texout[3] = texverts[b*2+1];
    texout[4] = texverts[c*2]; texout[5] = texverts[c*2+1];
    texout[6] = texverts[d*2]; texout[7] = texverts[d*2+1];
}

void display_init() {
	const int r[3] = {0,1,0};
	const int g[3] = {1,0,1};
	const int b[3] = {0,0,1};
	
	for(int x = 0; x < 3; x++) {
		for(int y = 0; y < 2; y++) {
			int cx = x*controlButtonWidth-1;
			int cy = y*(ztracebounds.height-controlButtonWidth);
			
            float texverts[8];
            
            switch(x) {
                case 0:
                    if (y)
                        texfill(texverts, arrowcoords[1], 2, 1, 0, 3);
                    else
                        texfill(texverts, arrowcoords[1], 1, 2, 3, 0);
                    break;
                case 1:
                    if (y)
                        texfill(texverts, arrowcoords[0], 2, 1, 0, 3);
                    else
                        texfill(texverts, arrowcoords[0], 1, 2, 3, 0);
                    break;
                case 2:
                    texfill(texverts, arrowcoords[0], 0, 1, 2, 3);
                    break;
            }
            
			traceBox.push(cx,cy,cx+controlButtonWidth,cy+controlButtonWidth, r[x], g[x], b[x], 1.0, texverts);
			
			cx = y*(ztracebounds.width-controlButtonWidth);
			cy = x*controlButtonWidth-1;
            
            float tx, ty;
            
            // Clumsily flip tex area
#if 0
            // Rotate
            tx = texverts[0]; ty = texverts[1];
            texverts[0] = texverts[2]; texverts[1] = texverts[3];
            texverts[2] = texverts[4]; texverts[3] = texverts[5];
            texverts[4] = texverts[6]; texverts[5] = texverts[7];
            texverts[6] = tx; texverts[7] = ty;*/
#endif
#if 0
            // Flip wrong
            tx = texverts[2]; ty = texverts[3];
            texverts[2] = texverts[6]; texverts[3] = texverts[7];
            texverts[6] = tx; texverts[7] = ty;
#endif
#if 1
            // Flip correct
            tx = texverts[0]; ty = texverts[1];
            texverts[0] = texverts[4]; texverts[1] = texverts[5];
            texverts[4] = tx; texverts[5] = ty;
#endif
            
			traceBoxUp.push(cx,cy,cx+controlButtonWidth,cy+controlButtonWidth, r[x], g[x], b[x], 1.0, texverts);
		}
	}
	traceBox.megaIndexEnsure(6);
}	

plateinfo *plate_construct(const char *fmt, int count, float r, float g, float b, platebehave behave)
{
	plateinfo *s = new plateinfo();
	s->arrays = new GLfloat *[count];
	s->quadCount = new int[count];
	s->texture = new GLuint[count];
	s->frames = count;
	s->r = r; s->g = g; s->b = b; s->behave = behave;
	s->color = packColor(r, g, b);
	s->slices = new slice *[count];
	
	s->coords = new coord [count];
	
	for(int c = 0; c < s->frames; c++) {
		char filename[FILENAMESIZE];
		
		snprintf(filename, FILENAMESIZE, fmt, c+1);
		s->coords[c] = atlascoords[filename];
//		ERR("888 %s %d,%d,%d,%d\n", filename, s->coords[c].x1, s->coords[c].x2, s->coords[c].y1, s->coords[c].y2);
		
		internalPath(filename, fmt, c+1);
		
		s->slices[c] = new slice();
		s->slices[c]->construct(filename, false); // Doesn't REALLY construct
		
		s->width = s->slices[c]->width;
		s->height = s->slices[c]->height; // If this isn't the same on each pass, you're doing it wrong
		
		s->arrays[c] = 0;
		s->quadCount[c] = 0;
	}
	
	s->reconstruct();
	
	return s;
}

inline float panfade(float _from, float _to) {
	float zoom = cos(M_PI/2 * (float)panct/pantt);
	float azoom = 1-zoom;
	float scale = -1;//36;
	float from = scale*_from;
	float to = scale*_to;
	return from*zoom+to*azoom;
}

void updateStencilColor(spaceinfo *s, int l) {
	stencilr = s->r[l]; stencilg = s->g[l]; stencilb = s->b[l]; stencila = 0.25;
	
	if (pantype == pan_deep && ((s->num == jumpman_s) ^ (exit_direction > 0))) {
		stencila = -panfade(0,1);
		if (s->num == jumpman_s)
			stencila = 1 - stencila;
		stencila *= 0.25;
	}
	
	if (ticks - FPS34 < rotYellingSince && (!restrictDraw || restrictDrawTo & chassisShape->layers)) {
		float fade = ticks - rotYellingSince; fade /= (FPS34);
		float unfade = (1-fade);
		float h,s,v, r2, g2, b2;
		
		RGBtoHSV( stencilr, stencilg, stencilb, &h, &s, &v );
		s = 1.0;
		if (isnan(h)) h = 0;
		else h = fmod(h + 180, 360);
		if (v < 0.1) v = 0.1;
		HSVtoRGB( &r2, &g2, &b2, h, s, v );
		
		stencilr = stencilr*fade+r2*unfade;
		stencilg = stencilg*fade+g2*unfade;
		stencilb = stencilb*fade+b2*unfade;
		stencila *= (2-fade);
	}
}

// From http://iphonedevelopment.blogspot.com/2008/12/gluperspective.html
inline void gluPerspective(float fovy, float paspect, float zNear, float zFar)
{
	// Start in projection mode.
	jcMatrixMode(GL_PROJECTION);
	jcLoadIdentity();
	float xmin, xmax, ymin, ymax;
	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;
	xmin = ymin * paspect;
	xmax = ymax * paspect;
	jcFrustumf(xmin, xmax, ymin, ymax, zNear, zFar);
}

void goPerspective() {
	jcMatrixMode(GL_PROJECTION);
	jcLoadIdentity();
	gluPerspective(60, 1/aspect, 0.5, 5);
	jcScalef(jumpman_r/750.0f, jumpman_r/750.0f, 1.0);
	jcMatrixMode(GL_MODELVIEW);
}

// This doesn't seem to get used anywhere?
void goSquare() {
	jcMatrixMode(GL_PROJECTION);
	jcLoadIdentity();
	gluPerspective(60, 1, 0.5, 5);
	jcScalef(jumpman_r/750.0f, jumpman_r/750.0f, 1.0);
	jcMatrixMode(GL_MODELVIEW);
}

// Note: If code is kept, must put MESA copyright notice in program
static void __gluMultMatrixVecd(const GLfloat matrix[16], const GLfloat in[4],
								GLfloat out[4])
{
    int i;
	
    for (i=0; i<4; i++) {
		out[i] = 
	    in[0] * matrix[0*4+i] +
	    in[1] * matrix[1*4+i] +
	    in[2] * matrix[2*4+i] +
	    in[3] * matrix[3*4+i];
    }
}
static void __gluMultMatricesd(const GLfloat a[16], const GLfloat b[16],
							   GLfloat r[16])
{
    int i, j;
	
    for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			r[i*4+j] = 
			a[i*4+0]*b[0*4+j] +
			a[i*4+1]*b[1*4+j] +
			a[i*4+2]*b[2*4+j] +
			a[i*4+3]*b[3*4+j];
		}
    }
}
static int __gluInvertMatrixd(const GLfloat m[16], GLfloat invOut[16])
{
    float inv[16], det;
    int i;
	
    inv[0] =   m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
	+ m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4] =  -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
	- m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8] =   m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
	+ m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
	- m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
    inv[1] =  -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
	- m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5] =   m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
	+ m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9] =  -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
	- m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
	+ m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
    inv[2] =   m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15]
	+ m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
    inv[6] =  -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15]
	- m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
    inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15]
	+ m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14]
	- m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
    inv[3] =  -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11]
	- m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
    inv[7] =   m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11]
	+ m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11]
	- m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
    inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10]
	+ m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];
	
    det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    if (det == 0)
        return GL_FALSE;
	
    det = 1.0 / det;
	
    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;
	
    return GL_TRUE;
}
GLint gluProject(GLfloat objx, GLfloat objy, GLfloat objz, 
				 const GLfloat modelMatrix[16], 
				 const GLfloat projMatrix[16],
				 const GLint viewport[4],
				 GLfloat *winx, GLfloat *winy, GLfloat *winz)
{
    float in[4];
    float out[4];
	
    in[0]=objx;
    in[1]=objy;
    in[2]=objz;
    in[3]=1.0;
    __gluMultMatrixVecd(modelMatrix, in, out);
    __gluMultMatrixVecd(projMatrix, out, in);
    if (in[3] == 0.0) return(GL_FALSE);
    in[0] /= in[3];
    in[1] /= in[3];
    in[2] /= in[3];
    /* Map x, y and z to range 0-1 */
    in[0] = in[0] * 0.5 + 0.5;
    in[1] = in[1] * 0.5 + 0.5;
    in[2] = in[2] * 0.5 + 0.5;
	
    /* Map x,y to viewport */
    in[0] = in[0] * viewport[2] + viewport[0];
    in[1] = in[1] * viewport[3] + viewport[1];
	
    *winx=in[0];
    *winy=in[1];
    *winz=in[2];
    return(GL_TRUE);
}
GLint gluUnProject(GLfloat winx, GLfloat winy, GLfloat winz,
				   const GLfloat modelMatrix[16], 
				   const GLfloat projMatrix[16],
				   const GLint viewport[4],
				   GLfloat *objx, GLfloat *objy, GLfloat *objz)
{
    GLfloat finalMatrix[16];
    GLfloat in[4];
    GLfloat out[4];
	
    __gluMultMatricesd(modelMatrix, projMatrix, finalMatrix);
    if (!__gluInvertMatrixd(finalMatrix, finalMatrix)) return(GL_FALSE);
	
    in[0]=winx;
    in[1]=winy;
    in[2]=winz;
    in[3]=1.0;
	
    /* Map x and y from window coordinates */
    in[0] = (in[0] - viewport[0]) / viewport[2];
    in[1] = (in[1] - viewport[1]) / viewport[3];
	
    /* Map to range -1 to 1 */
    in[0] = in[0] * 2 - 1;
    in[1] = in[1] * 2 - 1;
    in[2] = in[2] * 2 - 1;
	
    __gluMultMatrixVecd(finalMatrix, in, out);
    if (out[3] == 0.0) return(GL_FALSE);
    out[0] /= out[3];
    out[1] /= out[3];
    out[2] /= out[3];
    *objx = out[0];
    *objy = out[1];
    *objz = out[2];
    return(GL_TRUE);
}

void drawSplosions(spaceinfo *s, bool for_real) { // Someday will other things splode?
	GLfloat squareVertices[8];
	States(false, false); // Texture, Color
	jcVertexPointer(2, GL_FLOAT, 0, squareVertices);
	
	while (for_real && !s->splodes.empty() && s->splodes.back().ct >= s->splodes.back().tt) {
		s->splodes.pop_back();			
	}
	for(list<splode_info>::iterator b = s->splodes.begin(); b != s->splodes.end(); b++) {
		splode_info &i = *b;
		
		float zoom = (float)i.ct/i.tt;
		jcPushMatrix();
        if (savedControlRot & CGrav)
            jcRotatef(-(i.rot_pre)/M_PI*180,0,0,1);
		jcTranslatef(i.p.x,i.p.y,0.0);
		jcTranslatef(0,0,0);
        if (savedControlRot & CGrav)
            jcRotatef((i.rot_post)/M_PI*180,0,0,1);
        jcRotatef(( (savedControlRot & CGrav ? 0 : chassis->a) + M_PI)/M_PI*180,0,0,1);
		if (i.reflect)
			jcRotatef(180,0,1,0);
		jcColor4f(i.r,i.g,i.b, (1-zoom)*(1-zoom));      // Color Orange?
		for(int x = 0; x < i.sploding->width; x++) {
			for(int y = 0; y < i.sploding->height; y++) {
				if (i.sploding->pixel[x][y]) {
					float czoom = 1+100*zoom;
					float pzoom = 1 + 5*zoom;
					float x_ = x - i.sploding->width/2;
					float y_ = y - i.sploding->height/2;
					x_ *= czoom; y_ *= czoom;
					
					quadWrite(squareVertices, x_*scale, y_*scale, (x_ + pzoom)*scale, (y_ + pzoom)*scale);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				}
			}
		}
		jcPopMatrix();
		
		if (for_real) {
			i.ct++;
			i.adjust_sound();
		}
	}
	//glEnable(GL_DEPTH_TEST);
}

inline void Color3f(float r, float g, float b) {
	jcColor4f(r, g, b, 1.0);
}

void BlindColorTransform(float &r, float &g, float &b, unsigned int layers) {
	r /= 3; g /= 3; b /= 3;
	if (chassisShape->layers & layers) {
		r += 2/3.0; g += 2/3.0; b += 2/3.0;
	}
}	

inline void BlindColor3f(float r, float g, float b, unsigned int layers) {
	if (optColorblind)
		BlindColorTransform(r, g, b, layers);
	jcColor4f(r, g, b, 1.0);
}

float crandr, crandg, crandb;
int lastcrandattick = 0;
#define FADEFACTOR 3.0
inline void lavaColor() {
	if (ticks != lastcrandattick) {
		//	if (ticks >= lastcrandattick+5) { // Even this hurts people's eyes, so why bother
		lastcrandattick = ticks;
		crandr = crandr*((FADEFACTOR-1)/FADEFACTOR) + (float)random()/RANDOM_MAX/FADEFACTOR;
		crandg = crandg*((FADEFACTOR-1)/FADEFACTOR) + (float)random()/RANDOM_MAX/FADEFACTOR;
		crandb = crandb*((FADEFACTOR-1)/FADEFACTOR) + (float)random()/RANDOM_MAX/FADEFACTOR;
	}
	Color3f(crandr, crandb, crandg);
}

int currentJumpmanFrame() {
	int whichframe = 2;
	if (jumping) {
		whichframe = 1;
	} else if (input_power || fabs(overflowv) > 0.1) {
		int frames[5] = {0, 0, 3, 3, 2};
		unsigned int frame = (ticks - started_moving_at_ticks);
		frame /= 10;
		frame %= 5;
		whichframe = frames[frame];
	}
	return whichframe;
}

void updateObject(void *_shape, void *_data)
{
	cpShape *shape = (cpShape *)_shape;
	cpBody *body = shape->body;
	if (!body) return;
	enemy_info *data = (enemy_info *)shape->body->data;
	if (!data) return;
	spaceinfo *s = (spaceinfo *)_data;

	if (shape->collision_type == C_LOOSE) { // Shrapnel gets updated too, but it's special.
		cpPolyShape *poly = (cpPolyShape *)shape;
		cpVect *verts = poly->verts;
		for(int c = 0; c < 4; c++) {
			cpVect p = body->p, rot = body->rot;
			if (cam_fixed == s->camera) { // Doesn't fix the problem
				p = cpvunrotate(s->staticBody->rot, p); // Because we're awkwardly rotating the whole static vertex later, we have to compensate in the other direction...
				rot = cpvunrotate(s->staticBody->rot, rot);
				p.y = -p.y; // NO IDEA WHY THESE NEXT TWO LINES WORK AND IT SCARES ME
				rot.y = -rot.y;
			}
			cpVect v = cpvadd(p, cpvrotate(verts[c], rot));
			int base = data->megaOffset+ c*STATIC_STRIDE;
			//		ERR("%x of %d:]\t%d < %d < %d\n", shape, s->num, data->megaOffset, base, s->megaVertex.size());
			s->staticVertex[base] = v.x;
			s->staticVertex[base+1] = v.y;
		}
		return;
	}	
	
	if (data->megaOffset < 0 || data->megaOffset+4*MEGA_STRIDE > s->megaVertex.size()) return; // Paranoia?
	
	plateinfo *plate = pinfo[shape->collision_type];	
	if (!plate) return; // FIXME: Shrapnel have no plates but should be drawn anyway.
	
	static cpVect uoTexVertices[4];
	bool haveTex = false;

	if (plate) {
		bool reflect = false;
		int whichframe = 0;
		switch(shape->collision_type) {
			case C_JUMPMAN: {
				whichframe = currentJumpmanFrame(); 
				reflect = input_power_last_facing > 0;
				break;
			}
			case C_MARCH: case C_STICKY: case C_SWOOP: {
				reflect = cpvunrotate(shape->body->v,shape->body->rot).x < 0;
				whichframe = (ticks / 10) % 2;
				break;
			}
			case C_ING: {
				ing_info *info = (ing_info *)body->data;
				cpBodyResetForces(body);
				
				if (s->num == jumpman_s) {
					float dx = cpvdot( cpvsub(chassis->p, shape->body->p), shape->body->rot );
					if (s->repeat) {
						float altdx = dx + s->repeat_every * (dx>0 ? -1:1);
						if (fabs(altdx) < fabs(dx)) dx = altdx;
					}
					reflect = dx > 0;
					
					if (fabs(dx) > 36*2) {
						if (0 == info->runningSince)
							info->runningSince = ticks;
						whichframe = ((ticks-info->runningSince) / 20) % 2;
						
						if (!paused) { // Oh God, I'm servicing physics in the display handler?! WHAT AM I DOING?!?
							cpVect force = cpvmult(shape->body->rot, (reflect?1:-1)*750*body->m); // Is 500 a good number?
							cpVect offset = {0,0}; // TODO: Rotate
							cpBodyApplyForce(body, /*rot(*/force/*)*/, offset);
						}
					} else {
						info->runningSince = 0;
					}
				}
                break;
            }
            case C_BIRD: {
                //			ERR("V.X %f\n", body->v.x);
                if (body->v.x > 200)
                    whichframe = 2;
                else
                    whichframe = (ticks / 10) % 3;
                break;
            }                
            break;
            
            case C_BOMB2: case C_ANGRY: case C_HAPPY: { // In these cases the frames aren't frames, just alternate graphics
                if (optColorblind || s->layers > 0)
                    whichframe = 1;
            } break;				
		}
		coord &co = plate->coords[whichframe];
		if (reflect) {// Code duplication; ugly
			uoTexVertices[0] = cpv(co.x1, co.y2); uoTexVertices[1] = cpv(co.x1, co.y1);
			uoTexVertices[2] = cpv(co.x2, co.y1); uoTexVertices[3] = cpv(co.x2, co.y2); 
		} else {
			uoTexVertices[0] = cpv(co.x2, co.y2); uoTexVertices[1] = cpv(co.x2, co.y1);
			uoTexVertices[2] = cpv(co.x1, co.y1); uoTexVertices[3] = cpv(co.x1, co.y2); 
		}
		haveTex = true;
	}
	
	for(int c = 0; c < 4; c++) {
		cpVect v = cpvadd(shape->body->p, cpvrotate(plate->drawShape[c], body->rot));
		int base = data->megaOffset+ c*MEGA_STRIDE;
//		ERR("%x of %d:]\t%d < %d < %d\n", shape, s->num, data->megaOffset, base, s->megaVertex.size());
		s->megaVertex[base] = v.x;
		s->megaVertex[base+1] = v.y;
		if (haveTex) {
			s->megaVertex[base+3] = uoTexVertices[c].x;
			s->megaVertex[base+4] = uoTexVertices[c].y;
		}
	}
}	

void drawObject(void *_shape, void *_data)
{
	cpShape *shape = (cpShape *)_shape;
		
	if (!((shape->collision_type == C_NOROT) ^ !(drawingNoRots))) return; // Continue only if both false or both true
	
	cpBody *body = shape->body;
	spaceinfo *data = (spaceinfo *)_data;
	plateinfo *slice = pinfo[shape->collision_type];
	cpVect p = body->p;
	
	bool forceL = data->layers > 0;
	if (/*forceL && REMOVE */restrictDraw && (!(restrictDrawTo & shape->layers) || !C_FLOORTYPE(shape->collision_type)))
		return;
    
    bool suppressColor = false;
	
	if (restrictDrawObject && C_FLOORTYPE(shape->collision_type))
		return;
		
	unsigned int l = 0;
	if (forceL) {
		unsigned int layers = shape->layers;
		if (shape->collision_type == C_PAINT)
			layers = ~layers;
		
		for(int c = 0; c < MAXLAYERS; c++) {
			if ((1<<c) & layers) {
				l = c;
				break;
			}
		}
	}
	
	if (!slice && edit_mode == EWalls) { // We display invisible things differently in the editor
		if (C_INVISTYPE(shape->collision_type)) {
			slice = pinfo[C_OUTLINE];
		} else if (shape->collision_type == C_ENTRY) {
			slice = pinfo[C_OUTLINE2];
		} else if (!C_FLOORTYPE(shape->collision_type)) { // Cuz we don't draw C_FLOOR.
			slice = pinfo[C_UNKNOWN]; // This should only happen when a file is corrupt or was edited externally?
		}
	}
	
	bool have_slice = slice && slice->frames > 0;
	bool reflect = true;
	int whichframe = 0;
	
	switch(shape->collision_type) {
		case C_JUMPMAN: {			
			switch(jumpmanstate) {
				default: case jumpman_normal:
					whichframe = currentJumpmanFrame(); 
					reflect = input_power_last_facing > 0;
					if (!(invincible() && (ticks/5)%2))
						break;
				case jumpman_splode:
					have_slice = false; // Because of translucency sux0r, we'll draw this later
					break;
			}
#if FLICKER_DEBUG||TEXTURE_MODE_VISIBLE // Enable for textureMode marker
            Color3f(1,1,1);
            suppressColor = true;
#endif
		break;}
		case C_MARCH: case C_STICKY: case C_SWOOP: {
			reflect = cpvunrotate(shape->body->v,shape->body->rot).x < 0;
			whichframe = (ticks / 10) % 2;
			break;
		}
		case C_ING: {
			ing_info *info = (ing_info *)body->data;
		    cpBodyResetForces(body);
			
			if (data->num == jumpman_s) {
				float dx = cpvdot( cpvsub(chassis->p, shape->body->p), shape->body->rot );
				if (data->repeat) {
					float altdx = dx + data->repeat_every * (dx>0 ? -1:1);
					if (fabs(altdx) < fabs(dx)) dx = altdx;
				}
				reflect = dx > 0;
				
				if (fabs(dx) > 36*2) {
					if (0 == info->runningSince)
						info->runningSince = ticks;
					whichframe = ((ticks-info->runningSince) / 20) % 2;
					
					if (!paused) { // Oh God, I'm servicing physics in the display handler?! WHAT AM I DOING?!?
						cpVect force = cpvmult(shape->body->rot, (reflect?1:-1)*750*body->m); // Is 500 a good number?
						cpVect offset = {0,0}; // TODO: Rotate
						cpBodyApplyForce(body, /*rot(*/force/*)*/, offset);
					}
				} else {
					info->runningSince = 0;
				}
			}
			break;
		}
		case C_BIRD: {
			//			ERR("V.X %f\n", body->v.x);
			if (body->v.x > 200)
				whichframe = 2;
			else
				whichframe = (ticks / 10) % 3;
			break;
		}
			
		case C_PAINT: case C_ARROW: {
			forceL = true;
		} break;
			
#if 0		
		case C_BALL: case C_ANGRY: case C_HAPPY: {
			p.x -= fmod(p.x+2.4, 4.8);
			p.y -= fmod(p.y+2.4, 4.8);
		} break;
#else
		case C_BOMB2: case C_ANGRY: case C_HAPPY: { // In these cases the frames aren't frames, just alternate graphics
			if (optColorblind || forceL)
				whichframe = 1;
		} break;
#endif

		case C_LOOSE3: { // LOOSE2 and LOOSE3 are last-minute additions. Can you tell?
			if (edit_mode == EWalls) { // UNUSUALLY EGREGIOUS CODE DUPLICATION FOLLOWS
				float r = data->r[l], g = data->g[l], b = data->b[l];
				if ((ticks/(20 / 1))%2) {
					r = 1.0 - r; g = 1.0 - g; b = 1.0 - b;
					if ( ((r < 0.1) && (g < 0.1) && (b < 0.1)) // Flash shouldn't go to black or 50% gray, if it does it's not visible
						|| ((r > 0.4 && r < 0.6) && (g > 0.4 && g < 0.6) && (b > 0.4 && b < 0.6)) ) {
						r = fmin(r+0.5, 1); g = fmin(g+0.5, 1); b = fmin(b+0.5, 1);
					}
				}
				
				if (!forceL)
					Color3f(r,g,b);
				else
					BlindColor3f(r,g,b, shape->layers);
                
                suppressColor = true;
			}
		} break;
            
		case C_REENTRY: {
			if (edit_mode == EWalls) {
				cpPolyShape *poly = (cpPolyShape *)shape;
				int num = poly->numVerts;
				cpVect *verts = poly->verts;
				float lines[8];
                
                for(int i=0; i<num; i++){
					cpVect v = cpvadd(body->p, cpvrotate(verts[i], body->rot));
					cpVect v2 = verts[(i+2)%num];
					v.x += (v2.x > v.x ? 2.4 : -2.4);
					v.y += (v2.y > v.y ? 2.4 : -4.4); 
					
                    lines[i*2] = v.x;
                    lines[i*2+1] = v.y;
				} 
                
				glLineWidth(4);
				if (!forceL)
					Color3f(0.5,0.5,0.5);
				else
					BlindColor3f(data->r[l],data->g[l],data->b[l], shape->layers);                
                States(false, false);
                jcVertexPointer(2, GL_FLOAT, 0, &lines[0]);
                glDrawArrays(GL_LINE_LOOP, 0, 4);
				glLineWidth(1); // Would a caching LineWidth() be faster, or a terrible waste of time?
			}
		} break;
			
		case C_FLOOR: case C_LAVA: case C_LOOSE: case C_LOOSE2: case C_NOROT: {
			cpPolyShape *poly = (cpPolyShape *)shape;
			int num = poly->numVerts;
			cpVect *verts = poly->verts;
			
			if (shape->collision_type == C_NOROT)
				jcColor4f(stencilr,stencilg,stencilb,stencila);
			else if (shape->collision_type == C_LAVA)
				lavaColor();
			else if (edit_mode == EWalls && (shape->collision_type == C_LOOSE || shape->collision_type == C_LOOSE2)
					 && (ticks/(20 / (shape->collision_type == C_LOOSE2 ? 2 : 1)))%2) {
				float r = 1.0 - data->r[l], g = 1.0 - data->g[l], b = 1.0 - data->b[l];
				if ( ((r < 0.1) && (g < 0.1) && (b < 0.1)) // Flash shouldn't go to black or 50% gray, if it does it's not visible
					|| ((r > 0.4 && r < 0.6) && (g > 0.4 && g < 0.6) && (b > 0.4 && b < 0.6)) ) {
					r = fmin(r+0.5, 1); g = fmin(g+0.5, 1); b = fmin(b+0.5, 1);
				}
				
				if (!forceL)
					Color3f(r,g,b);
				else
					BlindColor3f(r,g,b, shape->layers);
			} else {
				if (!forceL)
					Color3f(data->r[l],data->g[l],data->b[l]);
				else
					BlindColor3f(data->r[l],data->g[l],data->b[l], shape->layers);
			}
			GLfloat vect[12];
			cpVect v[4];
			for(int i=0; i<num && i<4; i++)
				v[i] = cpvadd(body->p, cpvrotate(verts[i], body->rot));
			vect[0] = v[0].x; vect[1] = v[0].y;
			vect[2] = v[1].x; vect[3] = v[1].y;
			vect[4] = v[3].x; vect[5] = v[3].y;
			vect[6] = v[2].x; vect[7] = v[2].y;
			vect[8] = v[3].x; vect[9] = v[3].y;
			vect[10] = v[1].x; vect[11] = v[1].y;
			
            States(false, false);
			jcVertexPointer(2, GL_FLOAT, 0, vect);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		} break;
			
		default: {
			/*
			 jcPushMatrix();
			 glBegin(GL_QUADS);
			 Color3f(1.0,1.0,1.0);
			 for(int i=0; i<num; i++){
			 cpVect v = cpvadd(body->p, cpvrotate(verts[i], body->rot));
			 glVertex3f(v.x, v.y, 0);
			 }
			 glEnd();
			 jcPopMatrix();
			 */
		break;}
	}
	
	if (have_slice) {
		if (body->data) { // Seems inefficient?
			enemy_info *info = (enemy_info *)body->data;
			info->lastWhichFrame = whichframe;
			info->lastReflect = reflect;
		}

#if TEXTURE_DRAW
if (0&&textureMode) { // Not actually desirable anymore.
		GLfloat sliceVertices[8];
		GLfloat texVertices[8];
		coord &co = slice->coords[whichframe];
		quadWrite(texVertices, co.x2,co.y2,co.x1,co.y1);
		quadWrite(sliceVertices, -19.2,-19.2,19.2,19.2);

        States(true, false, false); // RTT splat -- no fog
		glBindTexture(GL_TEXTURE_2D, atlas);
		jcVertexPointer(2, GL_FLOAT, 0, sliceVertices);
		jcTexCoordPointer(2, GL_FLOAT, 0, texVertices);

        jcPushMatrix();
        jcTranslatef(p.x,p.y,0);
        jcRotatef(body->a/M_PI*180,0,0,1);
        if (reflect)
            jcRotatef(180,0,1,0);
        if (suppressColor) // Just for LOOSE3
            1;  // pass
        else if (!forceL)
            Color3f(slice->r,slice->g,slice->b);      // Color Orange
        else
            BlindColor3f(data->r[l],data->g[l],data->b[l], shape->collision_type != C_PAINT ? shape->layers : ~shape->layers);
    
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);		
} else
#endif
{
        States(false, false);
        jcVertexPointer(2, GL_FLOAT, 0, slice->arrays[whichframe]);
    
		jcPushMatrix();
		jcTranslatef(p.x,p.y,0);
		jcRotatef(body->a/M_PI*180,0,0,1);
		if (reflect)
			jcRotatef(180,0,1,0);
        if (suppressColor) // Just for LOOSE3
            1;  // pass
		else if (!forceL)
			Color3f(slice->r,slice->g,slice->b);      // Color Orange
		else
			BlindColor3f(data->r[l],data->g[l],data->b[l], shape->collision_type != C_PAINT ? shape->layers : ~shape->layers);
		
        glDrawArrays(GL_TRIANGLES, 0, 6*slice->quadCount[whichframe]);
} // #endif
		
		jcPopMatrix();
	}
	
#if DRAW_DEBUG
	if (!C_BALLTYPE(shape->collision_type)) {
		cpPolyShape *poly = (cpPolyShape *)shape;
		int num = poly->numVerts;
		cpVect *v = poly->verts;
		GLfloat vect[8];
		if (num > 4) num = 4;
		
		for(int i=0; i<num; i++){ // This will break in the presence of >8 vertices.
			cpVect nv = cpvadd(body->p, cpvrotate(v[i], body->rot));
			vect[i*2] = nv.x; vect[i*2+1] = nv.y;
		}
		
		jcColor4f(0.5f,0.5f,0.5f,1.0);
		jcVertexPointer(2, GL_FLOAT, 0, vect);
		glDrawArrays(GL_LINE_LOOP, 0, num);
	}
#endif
	//	if (shape->collision_type == C_MARCH)
	//		ERR("BODY %f %f\n", body->p.x, body->p.y);
	if (edit_mode == EPlayground && (shape->collision_type == C_MARCH || shape->collision_type == C_BALL) && body->p.x*body->p.x+body->p.y*body->p.y > 1000*1000*aspect*aspect*1.5*1.5) {
		//		ERR("KILL! %x\n", body);
		cpSpaceRemoveBody(data->space, body);
		cpSpaceRemoveShape(data->space, shape);
		//cpBodyFree(body); // TODO: MUST KILL
		//cpShapeFree(shape);
	}
}

void displayStaticFor(spaceinfo *s) { // Put in its own function so I can do a funny trick with stencils before drawing.
	bool mustNorot = s->has_norots && (s->num == jumpman_s || (pantype == pan_deep && s->num == jumpman_s + exit_direction));
	if (mustNorot) {
//		glEnable(GL_STENCIL_TEST);						// Enable Stencil Buffer For "marking" The Floor
//		glClear(GL_STENCIL_BUFFER_BIT);		
//		glStencilFunc(GL_NEVER, 1, ~0);						// Never passes (everything invisible), reference 1
//		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);				// We Set The Stencil Buffer To 1 Where We Draw Any Polygon
				
		// We have now built up a stencil buffer with the pixels to be drawn into and are ready to draw our content:

		int l = restrictDraw ? restrictDrawToInt : 0;  // This feature requires so much state to be tracked and none of it is used for anything else...
		updateStencilColor(s, l);
		
		drawingNoRots = true;
		cpSpaceHashEach(s->space->staticShapes, &drawObject, s);
		drawingNoRots = false;		
		
//		glStencilFunc(GL_EQUAL, 1, ~0);						// Passes when equal, reference 1
//		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);				// We don't care what happens to the stencil buffer anymore		
//		glDisable(GL_STENCIL_TEST);						// Enable Stencil Buffer For "marking" The Floor
	}	
	
	cpSpaceHashEach(s->space->staticShapes, &drawObject, s);
}

#if 1||GYRO_DEBUG
void drawOneLine(float y, float x) {
	jcLoadIdentity();
	jcTranslatef(x*(ztracebounds_orig.width), y*(ztracebounds_orig.height/2)+(ztracebounds_orig.height/2), 0);
	jcScalef(20,20,1);
	glDrawArrays(GL_LINE_STRIP, 0, 2);				
}

void drawGauge(float x) {
    if (!tilt)
        return;
    if (!tilt->enabled())
        return;
    
    jcColor4f(0.75,0.75,0.75, 1.0);
	drawOneLine(tilt->line(0), x);
	drawOneLine(tilt->line(1), x);
	
	jcColor4f(1.0,0.25,0.25, 1.0);
	drawOneLine(tilt->safe(0), x);
	drawOneLine(tilt->safe(1), x);
	
//	jcColor4f(0.5,0.5,0.5, 1.0);
//	drawOneLine(tilt->center(), x);
	
	jcColor4f(0.0,1.0,0.0, 1.0);
	drawOneLine(tilt->rawAt(), x);
    
	jcColor4f(1.0,1.0,0.0, 1.0);
	drawOneLine(tilt->at(), x);
}

void drawRaw(float a1, float a2, float x) {
    jcColor4f(0.5,0.5,0.5, 1.0);
    drawOneLine(0, x);
    
    float t = 0 == a1 && 0 == a2 ? -M_PI/2 : atan2(a1,a2);
    
    t += 3*M_PI/2; t = fmod(t, 2*M_PI); t -= M_PI;
    
    t /= M_PI;
    jcColor4f(1.0,1.0,0.0, 1.0);
    drawOneLine(t, x);
}
#endif

void displayLevel(spaceinfo *s, bool update_real = true, bool splode_real = true) {
	bool sploded = false; // Todo: Only once?
	
	bool forceBadGraphics = optColorblind && s->layers > 1;// || doingEnding;
    
    bool needDerotate = cam_fixed == s->camera && (savedControlRot & CGrav);

#if TEXTURE_DRAW
	if (textureMode && !forceBadGraphics) {		
		jcPushMatrix();
        
		if (cam_fixed == s->camera && !needDerotate) // Doesn't fix the problem
			jcRotatef(s->staticBody->a/M_PI * 180.0,0,0,1);
        
		float *megaVertex;
		
		if (s->has_norots && (s->num == jumpman_s || (pantype == pan_deep && s->num == jumpman_s + exit_direction))) {
			updateStencilColor(s, 0); // TODO: Something about multi-layer norots. force bad graphics, maybe?
            States(false, false);
			jcColor4f(stencilr,stencilg,stencilb,stencila);
			megaVertex = &(s->zoneVertex[0]);
			jcVertexPointer(2, GL_FLOAT, LAVA_STRIDE*sizeof(float), megaVertex);
			int drawTo = s->zoneVertex.size()*6/LAVA_STRIDE/4;
			glDrawElements(GL_TRIANGLES, drawTo, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);			
		}
		
		if (s->lavaVertex.size()) {
			lavaColor();
			megaVertex = &(s->lavaVertex[0]);
            States(false, false);
			jcVertexPointer(2, GL_FLOAT, LAVA_STRIDE*sizeof(float), megaVertex);
			int drawTo = s->lavaVertex.size()*6/LAVA_STRIDE/4;
			glDrawElements(GL_TRIANGLES, drawTo, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);
		}
                
		// STATIC
		
		megaVertex = &(s->staticVertex[0]);
        States(false, true);
		jcVertexPointer(2, GL_FLOAT, STATIC_STRIDE*sizeof(float), megaVertex);
		jcColorPointer(4, GL_UNSIGNED_BYTE, STATIC_STRIDE*sizeof(float), megaVertex+2);
		
		int drawTo = s->staticVertex.size()*6/STATIC_STRIDE/4;
		
		glDrawElements(GL_TRIANGLES, drawTo, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);
		
		// ACTIVE
		
        if (update_real)
            cpSpaceHashEach(s->space->activeShapes, &updateObject, s);
		
        States(true, true);
		glBindTexture(GL_TEXTURE_2D, atlas);
        
        // Do this here because blocks stay flat on their own if using Good Graphics
        jcPopMatrix();
        if (needDerotate) {
            jcPushMatrix();
            jcRotatef(-s->staticBody->a/M_PI * 180.0,0,0,1);        
        }
		
		if (s->kludgeVertex.size()) { // Can this be done without?
			megaVertex = &(s->kludgeVertex[0]);
			jcVertexPointer(2, GL_FLOAT, MEGA_STRIDE*sizeof(float), megaVertex);
			jcColorPointer(4, GL_UNSIGNED_BYTE, MEGA_STRIDE*sizeof(float), megaVertex+2);
			jcTexCoordPointer(2, GL_FLOAT, MEGA_STRIDE*sizeof(float), megaVertex+3);
			
			drawTo = s->kludgeVertex.size()*6/MEGA_STRIDE/4;
			
			glDrawElements(GL_TRIANGLES, drawTo, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);			
		}		
		
		megaVertex = &(s->megaVertex[0]);
		jcVertexPointer(2, GL_FLOAT, MEGA_STRIDE*sizeof(float), megaVertex);
		jcColorPointer(4, GL_UNSIGNED_BYTE, MEGA_STRIDE*sizeof(float), megaVertex+2);
		jcTexCoordPointer(2, GL_FLOAT, MEGA_STRIDE*sizeof(float), megaVertex+3);
		
		drawTo = s->megaVertex.size()*6/MEGA_STRIDE/4;
		
		glDrawElements(GL_TRIANGLES, drawTo, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);
    } else
#endif
	{
        if (needDerotate) { 
            jcPushMatrix();
            jcRotatef(-s->staticBody->a/M_PI * 180.0,0,0,1);
        }
        
		if (s->layers <= 0)
			displayStaticFor(s);
		else {
			for(int _c = s->layers-1; _c >= 0; _c--) {
				int c = (_c + editLayer) % s->layers;
				restrictDraw = true;
				restrictDrawToInt = c;
				restrictDrawTo = (1 << c);
				displayStaticFor(s);
				restrictDraw = false;
			}
			restrictDrawObject = true;
			cpSpaceHashEach(s->space->staticShapes, &drawObject, s);
			restrictDrawObject = false;
		}
		
		cpSpaceHashEach(s->space->activeShapes, &drawObject, s);
        
	}//#endif
    
    if (needDerotate)
        jcPopMatrix();
    	
	if (!s->splodes.empty()) {
		drawSplosions(s, splode_real);
		sploded = true;
	}
	
#if 0 && DRAW_DEBUG
	if (!s->trails.empty()) {
		drawTrails(s);
		sploded = true;
	}
#endif
}

struct DrawingLevel {
	spaceinfo *s;
#if FB_DRAW
	bool hasFb;
	GLuint repeater, repeatFb;
	cpVect basX, basY;
	DrawingLevel() : s(0), hasFb(false), repeater(0), repeatFb(0) {}
#endif
};
#define MAXVISIBLE 5

DrawingLevel drawing[MAXVISIBLE];

void display(void)
{	    
    EnableClientState(GLCS_VERTEX); // TODO: What state to set where?
    
    if (intro) { display_intro(); return; }
    
//	bool anyFbs = false;
    bool free_rotate = !NORMALANGLE || controlRot & CGrav 
        || (savedControlRot & CGrav && paused && edit_mode == ENothing);
	
	int dcount = 0;
	
	if (!optSplatter)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else
		glClear(GL_DEPTH_BUFFER_BIT);
	jcLoadIdentity();
		
	// #### #### ---- Zoom factor first:
	if (pantype != pan_dont) {
		jumpman_r = -panfade(panfr,pantr); // For some reason I made my fade return negative values...
		jumpman_r *= scan_r;
		goPerspective(); // Slightly overkill.
	} else {
		jumpman_r = rFor(level[jumpman_s]); // This is also probably unnecessary but I am being paranoid
		jumpman_r *= scan_r;
		if (rePerspective)
			goPerspective();		
	}
	
	// #### #### ---- Depth next:
	if (pantype != pan_dont) {	
        lastpanfadez = panfade(panfz,pantz); // Helps us know what to draw
		//		ERR("deep! %f\n", panfade(panfz,pantz));
		jcTranslatef(0, 0, lastpanfadez);
	} else {
		jcTranslatef(0, 0, -jumpman_d);
	}	
	
	// #### #### ---- Rotations + scan:

    if (!free_rotate)
        jcRotatef(90.0 * holdrot,0,0,1);
	
	jcTranslatef(scan.x, scan.y, 0);
	
	if (surplusSpin)
		jcRotatef(surplusSpin/M_PI*180.0, 0,0,1);
    
	if (free_rotate) { // Fixed rotation has to be "undone" in free rotate mode
/*        // FIXME: I DON'T THINK I NEED ANY OF THIS
        if (pantype == pan_deep &&
            (  (want_rotting_s >= 0 && 
                (level[want_rotting_s].camera != cam_fixed || level[want_rotting_s].rots == 0 || level[want_rotting_s].rots == 1))
             || (rotting_s != jumpman_s && 
                 (level[rotting_s].camera != cam_fixed || level[rotting_s].rots == 0 || level[rotting_s].rots == 1)))) { // FIXME: This is very probably wrong
                float nroten =   level[rotting_s].staticBody->a; // rotting_s will usually do nothing
                float nroten2 =  0; 
                                
                while (nroten2 - nroten > M_PI) nroten2 -= 2*M_PI;
                while (nroten - nroten2 > M_PI) nroten  -= 2*M_PI;
                
                jcRotatef(panfade(nroten, nroten2)/M_PI * 180.0,0,0,1);
            } else {
                jcRotatef(-level[rotting_s].staticBody->a/M_PI * 180.0,0,0,1); // rotting_s always == jumpman_s when !pan_deep... right?
            } */
	} else if (!(cam_fixed == level[jumpman_s].camera)) {
		if (pantype != pan_deep) {
            if (!free_rotate)
                jcRotatef(-roten/M_PI * 180.0,0,0,1);
		} else {
			float nroten =   exit_grav_adjust || free_rotate ? 0 : frot(0); // round to something reasonable
            float nroten2 = !exit_grav_adjust || free_rotate ? surplusSpin : frot(0);
            
			jcRotatef(panfade(nroten, nroten2)/M_PI * 180.0,0,0,1); // I guess surplusspin here so there's no jump when it's removed?
		}
	}
    
// Before we dislodge the earth from the center of the universe, take a break and do some framebuffer maintenance.
// There are no framebuffers anymore, so this just means making a list of spaces to draw. Remove this later?
	for(int c = level.size()-1; c >= 0 && dcount < MAXVISIBLE; c--) { // FIXME: Forward or backward?
		spaceinfo* const s = &(level[c]);
		if (!okayToDraw(s->deep))
			continue;
		drawing[dcount].s = s;
		
        s->tryLoad();
        
#if FB_DRAW
		if (s->repeat) {
			GLfloat model_view[16];
			GLfloat projection[16];
			GLint viewport[4];	
			GLfloat tempX, tempY, tempZ, tempX2, tempY2;
			if (cam_fixed == s->camera) {
				jcPushMatrix();
				jcRotatef(s->staticBody->a/M_PI*180,0,0,1);
			}
            jcMatrixFetch(projection, model_view, viewport);
			if (cam_fixed == s->camera)
				jcPopMatrix();
			
			// TODO: I totally bet I could record buffZ rather than tempZ here and save a gluProject later
			gluProject(-s->repeat_every,-s->repeat_every,s->deep, model_view, projection, viewport, &tempX, &tempY, &tempZ); // Previously s->deep was 0 here
			gluProject(s->repeat_every,s->repeat_every,s->deep, model_view, projection, viewport, &tempX2, &tempY2, &tempZ);
			cpVect d = cpvsub(cpv(tempX, tempY), cpv(tempX2, tempY2));
			float extent = fmax(fabs(d.x), fabs(d.y));
			
			if (extent < FB_SIZE) { // We're going to draw into the framebuffer
				drawing[dcount].hasFb = true;
				
				if (!drawing[dcount].repeatFb) { // We need to create the framebuffer
					glGenTextures(1, &drawing[dcount].repeater);
					
					glGenFramebuffersOES(1, &drawing[dcount].repeatFb);
					
					glBindFramebufferOES(GL_FRAMEBUFFER_OES, drawing[dcount].repeatFb);		
					
#error this next line is ES2 incompatible
					  Enable(GLE_TEXTURE);
					glBindTexture(GL_TEXTURE_2D, drawing[dcount].repeater);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FB_SIZE, FB_SIZE, 0,
										 			 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					
					glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, 
													  						  GL_TEXTURE_2D, drawing[dcount].repeater, 0);
				} else {
					glBindFramebufferOES(GL_FRAMEBUFFER_OES, drawing[dcount].repeatFb);
				}
				// TODO: Scissors?
				glViewport(-int(normalWidth-FB_SIZE)/2, -int(normalHeight-FB_SIZE)/2, normalWidth, normalHeight);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				
				jcPushMatrix();
				jcTranslatef(0, 0, s->deep);	
				displayLevel(s);
				jcPopMatrix();
								
//				DELETETHISFLAG = true;
//				ERR("[%d Y - %f,%f]\t", s->deep, d.x, d.y); // Delete me
			} else
				drawing[dcount].hasFb = false;
			
//			ERR("%d: DIST (%f,%f) %f -- re %f, x %f->%f; y %f->%f; z %f\n", s->deep, d.x, d.y, cpvlength(d), s->repeat_every, tempX2, tempX, tempY2, tempY, tempZ);
		}		
#endif
		
		dcount++;
}
	
    // There are no framebuffers anymore, so this just means making a list of spaces to draw.
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, normalFb);	// Now that all render to texture is done		
	glViewport(0, 0, normalWidth, normalHeight); // Why here exactly, though?
	
//	if (DELETETHISFLAG) ERR("\n"); // Delete me
	
	// #### #### ---- Translations
#if NORMALANGLE
	jcTranslatef(rescan.x, rescan.y, 0);
#endif
	
	if (pantype != pan_dont) {	
		jcTranslatef(panfade(panf.x,pant.x), panfade(panf.y,pant.y), 0);
	} else if (cam_fixed == level[jumpman_s].camera || edit_mode == EWalls) {
		// Do nothing
	} else {			
		cpVect p = chassis->p;
		
		if (level[jumpman_s].repeat && level[jumpman_s].reprot) {
			unsigned int reprot = tiltrightnow();
			unsigned int rotate = reprot&REPROTROT;
			unsigned int reflect = reprot&REPROTREF;
			if (reflect)
				p.x = -p.x;
			p = cpvrotate(p, cpvforangle(rotate*(M_PI/2)));
			//			ERR("rescan %lf,%lf : rot %d ref %d p [%f,%f]->[%f,%f]\n", rescan.x/level[jumpman_s].repeat_every, rescan.y/level[jumpman_s].repeat_every, rotate, reflect, chassis->p.x, chassis->p.y, p.x, p.y);
		}		
		
		jcTranslatef(-p.x, -p.y, 0);
	}
	
	// #### #### ---- Done! Start drawing.
		
	  Disable(GLE_DEPTH);
		
	if (edit_mode == EWalls || edit_mode == EZoom || edit_mode == EAngle || edit_mode == EPlayground || controlMove == CFollow) // So that we can determine how close presses are to Jumpman
		calculateCoordinates();
	
#if FLICKER_DEBUG
	textureMode = (ticks/FLICKER_DEBUG) % 2;
#endif
	
	for(int c = 0; c < dcount; c++) { // FIXME: Forward or backward?
		spaceinfo * const s = drawing[c].s;
		if (!okayToDraw(s->deep))
			continue;
		jcPushMatrix();
		
        // #### #### ---- Draw level
        
		jcTranslatef(0, 0, s->deep);	
		
		if (s->repeat) { // Geez this is actually really complicated
			GLfloat model_view[16];
			GLfloat projection[16];
			GLint viewport[4];	
			GLfloat buffZ, tempX, tempY, tempZ;

            bool needMeasureRot = cam_fixed == s->camera && !(savedControlRot & CGrav);
            
			if (needMeasureRot) {
				jcPushMatrix();
				jcRotatef(-s->staticBody->a/M_PI*180,0,0,1);
			}
            jcMatrixFetch(projection, model_view, viewport);
			glGetFloatv(GL_MODELVIEW_MATRIX, model_view);
			glGetFloatv(GL_PROJECTION_MATRIX, projection);
			glGetIntegerv(GL_VIEWPORT, viewport);
			if (needMeasureRot)
				jcPopMatrix();
			
			GLfloat x[4], y[4];
			
			gluProject(0,0,0, model_view, projection, viewport, &tempX, &tempY, &buffZ); // Previously s->deep was 0 here
			gluUnProject(0,0,buffZ, model_view, projection, viewport, &x[0], &y[0], &tempZ);
			gluUnProject(0,ztracebounds_orig.height,buffZ, model_view, projection, viewport, &x[1], &y[1], &tempZ);
			gluUnProject(ztracebounds_orig.width,0,buffZ, model_view, projection, viewport, &x[2], &y[2], &tempZ);
			gluUnProject(ztracebounds_orig.width,ztracebounds_orig.height,buffZ, model_view, projection, viewport, &x[3], &y[3], &tempZ);
								
#if FB_DRAW
			GLfloat sliceVertices[8]; // Might not get used
			GLfloat texVertices[8];

			if (drawing[c].hasFb) {
				gluProject(s->repeat_every,0,0, model_view, projection, viewport, &drawing[c].basX.x, &drawing[c].basX.y, &tempZ); // Previously s->deep was 0 here
				gluProject(0,s->repeat_every,0, model_view, projection, viewport, &drawing[c].basY.x, &drawing[c].basY.y, &tempZ); // Previously s->deep was 0 here
				drawing[c].basX.x -= tempX; drawing[c].basX.y -= tempY; 
				drawing[c].basY.x -= tempX; drawing[c].basY.y -= tempY;
				
				quadWrite(texVertices, 0,0,1,1);
				quadWrite(sliceVertices, -FB_SIZE/2,-FB_SIZE/2,FB_SIZE/2,FB_SIZE/2);
				
				  EnableClientState(GLCS_TEXTURE);
				  Enable(GLE_TEXTURE);
				glBindTexture(GL_TEXTURE_2D, drawing[c].repeater);
				jcVertexPointer(2, GL_FLOAT, 0, sliceVertices);
				jcTexCoordPointer(2, GL_FLOAT, 0, texVertices);
				jcColor4f(1,1,1,1);				
				
				jcMatrixMode(GL_PROJECTION);
				jcPushMatrix();
				jcLoadIdentity();
				jcOrthof( 0, ztracebounds_orig.width, 0, ztracebounds_orig.height, -1, 1);
				jcMatrixMode(GL_MODELVIEW);
				jcLoadIdentity();						
				jcTranslatef(tempX, tempY, 0);
			}
#endif
				
			int x1, y1, x2, y2;
			{
				double _x1, _y1, _x2, _y2;
				for(int c = 0; c < 4; c++) {
					if (0 == c || x[c] < _x1) _x1 = x[c];
					if (0 == c || y[c] < _y1) _y1 = y[c];
					if (0 == c || x[c] > _x2) _x2 = x[c];
					if (0 == c || y[c] > _y2) _y2 = y[c];
				}
				x1 = floorf(_x1/s->repeat_every + 0.5);
				x2 = floorf(_x2/s->repeat_every + 0.5);
				y1 = floorf(_y1/s->repeat_every + 0.5);
				y2 = floorf(_y2/s->repeat_every + 0.5);
				
//								ERR("WILLDRAW %d: [%lf,%lf]->[%lf,%lf] == [%d,%d]->[%d,%d]\n", s->num, x[0]/s->repeat_every, y[0]/s->repeat_every, x[3]/s->repeat_every, y[3]/s->repeat_every, x1, y1, x2, y2);
			}			
			
			for(int y = y1; y <= y2; y++) {
				for(int x = x1; x <= x2; x++) {
					//int rotby = y%2?3-((x + (y/2)*2)%4):(x + (y/2)*2)%4;
					
					int rr = s->tiltfor(x,y);
					int rotby = rr&REPROTROT;
					int reflect = rr&REPROTREF;
					
					cpVect offset;
#if FB_DRAW
						
					if (drawing[c].hasFb)
						offset = cpvadd( cpvmult(drawing[c].basX, x), cpvmult(drawing[c].basY, y) );
					else
#endif
						offset = cpv(x*s->repeat_every, y*s->repeat_every);
							
                    // So this next bit is wrong, wrong, wrong. In the presence of reflect it will mess up.
                    // However we can correct this messing-up using a weird trick below.
                    // ...but this comes with a cost too. The weird trick only works if the camera is pointed
                    // squarely at 0,0 *OR* staticBody->rot is 0. I assume staticBody->rot will be 0
                    // unless we are at jumpman_s or rotting_s; this is a false assumption but maybe the user
                    // won't notice the difference. This will fail basically if you used fixed camera + an unexit,
                    // or if you watch carefully in the split second during a grav_adjust.
                    // Update: This is probably all mooted by the new CGrav camera scheme?
					if ( (s->num == jumpman_s || s->num == rotting_s) && cam_fixed == s->camera && !(savedControlRot & CGrav)) {
						cpVect fixedRotateBy = s->staticBody->rot;
						if (reflect)
							fixedRotateBy = cpvneg(fixedRotateBy);
						offset = cpvrotate(offset, fixedRotateBy);						
					}
					jcPushMatrix();
					jcTranslatef(offset.x, offset.y, 0);
					
					if (rotby)
						jcRotatef(90*rotby, 0,0,1);
					
					if (reflect) {
						if ( (s->num == jumpman_s || s->num == rotting_s) && cam_fixed == s->camera && !(savedControlRot & CGrav))
							jcRotatef(2*cpvtoangle(s->staticBody->rot)/(M_PI)*180, 0,0,1); // "Weird trick"
						jcRotatef(180,0,1,0);
					}					
					
#if FB_DRAW
					if (drawing[c].hasFb) {						
						glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);								
					} else {
#endif
						displayLevel(s, y == y1 && x == x1, y == y2 && x == x2); // Only first tile does updates, only final tile does splodes
#if FB_DRAW
				}
#endif
					jcPopMatrix();
				}
			}
				
#if FB_DRAW
			if (drawing[c].hasFb) {						
				jcMatrixMode(GL_PROJECTION);
				jcPopMatrix();
				jcMatrixMode(GL_MODELVIEW);
			}
#endif
				
		} else
			displayLevel(s);
		
		jcPopMatrix();
	}

    if (doingEnding) { // Ending-- image and "tick"
        if (mountain) {
            jcPushMatrix();
        
#define BPM 120
#define SPAN ((44100*60)/BPM)
            int mountainat = (endingAt/(SPAN)*2 - 256 - 16);
            if (mountainat > 14) mountainat = 14;
            
            jcScalef(3.0/4, 3.0/4, 1);
            
            if (level.size() > 0)
                jcRotatef(-level[0].staticBody->a/M_PI*180,0,0,1);

            jcTranslatef(15*4.8, mountainat*4.8, -1);
            
            float is = 4.8 * 512 / 2;
            
            GLfloat sliceVertices[8];
            GLfloat texVertices[8];
            quadWrite(texVertices, 1,1,0,0);
            quadWrite(sliceVertices, -is,-is,is,is);        
            
            States(true, false, false);    // Mountain draw -- no fog
            jcColor4f(1,1,1,1);
            glBindTexture(GL_TEXTURE_2D, mountain); // what would happen if I didn't do this?
            jcVertexPointer(2, GL_FLOAT, 0, sliceVertices);
            jcTexCoordPointer(2, GL_FLOAT, 0, texVertices);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            jcPopMatrix();
        }
        
        // TODO: endingAtMode == 9? Or is that handled now?
		
		endingTickMainThread();
	}    
    
	// #### #### ---- DRAW EXTRA-LEVEL STUFF HERE
	// TODO: Factor out
	
    // #### #### ---- Grid
	if (edit_mode == EWalls && drawGrid) {	
        float fcurrent = roten / (M_PI/4);
        int current = (fabs(fcurrent) + 0.5); // WTF				
        
        //ERR("CURRENT %f %d %d\n", fcurrent, current, current%2);
        current %= 2;
        current += (editLayer * 2);        
        
        if (current < editSlice.size()) { // Don't proceed if level is malformed.            
            int c = current; // TODO: NAME SOMETHING ELSE.
            int px = editSlice[c]->width/2, py = editSlice[c]->height/2;
            int ox = editSlice[c]->width%2, oy = editSlice[c]->height%2;
            
            float xoff = ox ? 18 : 0, yoff = oy ? 18 : 0; 
            float xfrom = -36*px-xoff, xto = 36*px+xoff;
            float yfrom = -36*py-yoff, yto = 36*py+yoff;
            
            vector<float> grid; // TODO: PREALLOCATE ALL THIS
            int glen = 4*(px + ox + py + oy + 1);
            grid.resize(2*(glen + 1));
            int i = 0;
                    
            for(int c = 0; c < 2; c++)
                grid[i++] = 0;
            for(float c = xfrom; c <= xto && i < grid.size(); c+=36) {
                grid[i++] = c; grid[i++] = yfrom;
                grid[i++] = c; grid[i++] = yto;
            }
            for(float c = yfrom; c <= yto && i < grid.size(); c+=36) {
                grid[i++] = xfrom; grid[i++] = c;
                grid[i++] = xto; grid[i++] = c;
            }

            Disable(GLE_DEPTH); // Dupe            
            States(false, false);
            jcVertexPointer(2, GL_FLOAT, 0, &grid[0]);
            
            jcPushMatrix();
            jcTranslatef(0,0,jumpman_d-1);
            if (c%2) jcRotatef(45,0,0,1);
            
            jcColor4f(72.0/255, 99.0/255, 128.0/255, 0.5);
            glDrawArrays(GL_LINES, 1, glen);
            jcColor4f(0.0, 1.0, 0.0, 1.0);
            glDrawArrays(GL_POINTS, 0, 1); // POINT SIZE?
            jcPopMatrix();
        }
	}	
    // #### #### ---- C_LINE "x"
    if (edit_mode == EWalls && haveLineStart) { // Minor DRY issues -- Maybe combine with drawgrid?
        double fcurrent = roten / (M_PI/4);
        int current = (fabs(fcurrent) + 0.5); // WTF				
        current %= 2;
        current += (editLayer * 2);
        if (!(current >= editSlice.size())) {
            int px = editSlice[current]->width/2, py = -editSlice[current]->height/2;
            int ox = editSlice[current]->width%2, oy = editSlice[current]->height%2;
            px -= lineStartX; py += lineStartY;
            
            double xoff = ox ? 18 : 0, yoff = oy ? 18 : 36; // Without the -36, X is one square off. Why? 
            double xfrom = -36*px-xoff;
            double yfrom = -36*py-yoff;
            
            float lines[8] = {xfrom, yfrom, xfrom+36, yfrom+36, xfrom, yfrom+36, xfrom+36, yfrom};
            

            States(false, false);
            jcVertexPointer(2, GL_FLOAT, 0, &lines[0]);
            jcPushMatrix();
            jcTranslatef(0,0,jumpman_d-1);
            if (current%2) jcRotatef(45,0,0,1);
            jcColor4f(1-72.0/255, 1-99.0/255, 1-128.0/255, 0.5);
            glDrawArrays(GL_LINES, 0, 4);
            jcPopMatrix();
        }
    }
        
	
	if (drawEsc || optAxis || screentraces_count || drawingFloaters.size()>0 || controlMove == CButtonMove
        || controlRot == CButtonRot || edit_mode == EAngle) { // Anything that uses 1:1 projection
		jcMatrixMode(GL_PROJECTION);
		jcLoadIdentity();
		jcOrtho( 0, ztracebounds_orig.width, 0, ztracebounds_orig.height, -1, 1);
		jcMatrixMode(GL_MODELVIEW);
		
        // #### #### ---- ESC arrow
		if (drawEsc || doingEnding) { // The doingEnding would maybe be better as a justCalcEsc, maybe?			
            unsigned int userot = holdrot; // Notice the happy coincidence of unsigned int vs modulo 4. :(
            if (controlRot & CGrav && tilt) { // I hate everything about this
                float at = tilt->center();
                userot = int( (at + 1)*4+0.5 ); // Casting float directly to uint produces 0
                
                if (userot%8 == 1 || userot%8 == 5) // "Prefer" the wide side
                    userot++;

//                if (0 == userot%2) { // Used to make triangles "sticky" at 90 deg angles but it turned out irritating in practice
                    userot /= 2; userot = 4 - userot; userot %= 4;
                    if (userot != lastdrawescat) {
                        lastdrawescat = userot;
                    }
//                }
                userot = lastdrawescat;
            } else
                lastdrawescat = holdrot; // Starting to overload this thing's meaning I guess
                        
            if (drawEsc) {
                const int x = 8; const int y = 4;
                const GLfloat sliceVertices[6] = {x/2,0,x,y,0,y}; // {0,0,x,0,x/2,y};

                States(false,false);
                jcVertexPointer(2, GL_FLOAT, 0, sliceVertices);

                jcLoadIdentity();
                switch (userot) { // Like seriously is this really the only way to do this
                    case 1:
                        escAt = cpv( y + 2, (ztracebounds_orig.height - x) / 2 );
                        break;
                    case 2:
                        escAt = cpv( (ztracebounds_orig.width - x) / 2, y + 2 );
                        break;
                    case 3:
                        escAt = cpv( ztracebounds_orig.width - y - 2, (ztracebounds_orig.height - x) / 2 );
                        break;
                    default: // "Case 0"
                        escAt = cpv( (ztracebounds_orig.width - x) / 2, ztracebounds_orig.height - y - 2 );
                        break;
                }
                
                jcTranslatef(escAt.x, escAt.y, 0);
                jcRotatef(90.0 * userot,0,0,1);
                escHave = true;
                
                float alpha = 1.0;
                if (ticks-escSince < TRACELIFE)
                    alpha = float(ticks-escSince)/TRACELIFE;

#define ESC_BRIGHTNESS 0.5
                
                if (!doingEnding)
                    jcColor4f(1,1,1,ESC_BRIGHTNESS * alpha);      // Color Gray?
                else if ((ticks / 30) % 2)
                    jcColor4f(0,0,0,1);      // Color Black-- blink in ending
                else
                    jcColor4f(0,0,0,0);
                
                glDrawArrays(GL_TRIANGLES, 0, 3);		
            }
		}
		
        // #### #### ---- Tilt tracers
        if (controlRot & CTilt && tilt) {
//            const float top = tilt.at();
//            const float csafe[2] = {tilt.safe(0), tilt.safe(1)};
//            const float cline[2] = {tilt.line(0), tilt.line(1)};

            for(int c = 0; c < 2; c++) {
                const float alpha0 = tilt->shadow(c);
                const int flarelen = ticks-tiltFlare[c];

                if (alpha0 || (flarelen < (TRACELIFE/1))) {
                    const float flare = (flarelen > (TRACELIFE/1) ? 1 : (float)flarelen/(TRACELIFE/1));
                    const float alpha = alpha0 + (1-alpha0)*(1-flare);
                    const bool side = (c==0)^(holdrot==0);
                    const float drawreg = (side?1:-1) * ztracebounds.width / 6;
                    const float base = (side?0:ztracebounds.width);
                    const unsigned int colorTo = packColor(flare, 1, flare, 0);
                    const unsigned int colorFrom = packColor(flare, 1, flare, alpha);
                    
                    GLfloat sliceVertices[8]; // 13-18 lines of code to draw a rectangle. I don't even know.
                    if (!holdrot)
                        straightWrite(sliceVertices, base,0,base+drawreg,ztracebounds.height);	
                    else
                        straightWrite2(sliceVertices, 0,base,ztracebounds.height, base+drawreg);	
                    const GLfloat colorVertices[4] = {*((float *)&colorFrom),*((float *)&colorFrom),*((float *)&colorTo),*((float *)&colorTo)};	//Could be const but maybe this way they'll at least be close on the stack?
                    quadpile::megaIndexEnsure(2);

                    States(false, true);
                    jcVertexPointer(2, GL_FLOAT, 2*sizeof(float), sliceVertices);
                    jcColorPointer(4, GL_UNSIGNED_BYTE, 0, colorVertices);
                    jcLoadIdentity();
                    glDrawElements(GL_TRIANGLES, 8, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);
                }
            }
        }
        
        // #### #### ---- Finger tracers
		if (screentraces_count) {
#if TEXTURE_DRAW
			GLfloat sliceVertices[8];
			GLfloat texVertices[8];
			
			coord &co = arrowcoords[2];
			straightWrite(texVertices, co.x2,co.y2,co.x1,co.y1);
			straightWrite(sliceVertices, -1,-1,1,1);	
            GLfloat content[MEGA_STRIDE*4*FTMAX];
#else
			jcVertexPointer(2, GL_FLOAT, 0, traceCircle);
#endif
            
			while (screentraces_count && screentraces_back().ct >= TRACELIFE) {
				screentraces_pop_back();
			}
            
            int c;
			for(c = 0; c < screentraces_count; c++) {
				trace_info &i = screentraces[(screentraces_base+c)%FTMAX];
                
                float progress = (float)i.ct/TRACELIFE;
                float alpha = 1 - progress;
                float rad = 80;
                
                rad *= i.rad;
#if TEXTURE_DRAW
                rad *= progress;
                
                for(int d = 0; d < 4; d++) {
                    unsigned int color = packColor(i.r, i.g, i.b, alpha);
                    content[(c*4+d)*MEGA_STRIDE] =   i.at.x + sliceVertices[d*2]*rad;
                    content[(c*4+d)*MEGA_STRIDE+1] = i.at.y + sliceVertices[d*2+1]*rad;
                    content[(c*4+d)*MEGA_STRIDE+2] = *((float *)&color);
                    content[(c*4+d)*MEGA_STRIDE+3] = texVertices[d*2];
                    content[(c*4+d)*MEGA_STRIDE+4] = texVertices[d*2+1];
                }
#else                
                jcTranslatef(i.at.x, i.at.y, 0); // NOTE: THIS MAY NOT STILL WORK
                jcScalef(rad*progress,rad*progress,1);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);					
#endif
                i.ct++;
            }	

#if TEXTURE_DRAW
            int drawTo = c*6; // WHY 6? I DON'T REALLY KNOW, AND THAT SCARES ME
            quadpile::megaIndexEnsure(drawTo);
                            
            States(true, true, false);  // Finger traces -- no fog
            glBindTexture(GL_TEXTURE_2D, arrow_atlas);
            jcVertexPointer(2, GL_FLOAT, MEGA_STRIDE*sizeof(float), content);
            jcColorPointer(4, GL_UNSIGNED_BYTE, MEGA_STRIDE*sizeof(float), content+2);
            jcTexCoordPointer(2, GL_FLOAT, MEGA_STRIDE*sizeof(float), content+3);

            jcLoadIdentity();
            glDrawElements(GL_TRIANGLES, drawTo, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);
#endif
		}
		
        // #### #### ---- Buttons
		if (controlMove == CButtonMove || controlRot == CButtonRot) {
#if 1
			quadpile &whichBox = holdrot ? traceBox : traceBoxUp;
			unsigned char *boxValues = (unsigned char *)&whichBox[0];
			for(int i = 0; i < 6; i++) {
				int len = ticks-buttonFlare[i];
				float alpha = len > TRACELIFE ? 0.25 : 0.75-(float)len/TRACELIFE/2;
#if 1
                // Dubious means of indicating "can't rot" visually in buttons mode
                if (i < 2 && rotting_s < level.size()) {
                    if (level[rotting_s].rots == 0 || level[rotting_s].rots == 1
                        || (i%2 && (level[rotting_s].dontrot & 0x02))
                        || (!(i%2) && (level[rotting_s].dontrot & 0x80))
                        )
                        alpha = 0;
                }
#endif
				for(int v = 0; v < 4; v++)
					boxValues[2*sizeof(float) + (5*sizeof(float) * (i*4+v)) + 3] = alpha*255;
			}
			int start = 0;
			int len = 6*6;
			if (controlRot != CButtonRot) {
				start += 2*3*4;
				len -= 2*6;
			}
			if (controlMove != CButtonMove)
				len -= 4*6;
			jcLoadIdentity();
            
            States(true, true, false); // Control boxes-- no fog   
            glBindTexture(GL_TEXTURE_2D, arrow_atlas); // what would happen if I didn't do this?
            jcVertexPointer(2, GL_FLOAT, MEGA_STRIDE*sizeof(float), &whichBox[start]);
			jcColorPointer(4, GL_UNSIGNED_BYTE, MEGA_STRIDE*sizeof(float), &whichBox[start]+2);
            jcTexCoordPointer(2, GL_FLOAT, MEGA_STRIDE*sizeof(float), &whichBox[start]+3);
			glDrawElements(GL_TRIANGLES, len, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);
#else
			jcVertexPointer(2, GL_FLOAT, 0, traceBox);
			const int r[3] = {0,1,0};
			const int g[3] = {1,0,1};
			const int b[3] = {0,0,1};
			
			for(int y = 0; y < 2; y++) {
				jcLoadIdentity();
				jcTranslatef(-1, y*(ztracebounds.height-controlButtonWidth), 0);
				
				for(int x = 0; x < 3; x++) {
					if ((controlMove == CButtonMove && x>0) || (controlRot==CButtonRot && x==0)) {
						int i = x+y*3;
						int len = ticks-buttonFlare[i];
						float alpha = len > TRACELIFE ? 0.25 : 0.75-(float)len/TRACELIFE/2;
						jcColor4f(r[x],g[x],b[x], alpha);
						glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					}
					if (x < 2)
						jcTranslatef(controlButtonWidth,0,0);
				}
			}
#endif
		}
        
        if (ticks - FPS34 < rotYellingIconSince) { // Note: Isn't included in the "anything..." above because it can only be drawn when drawEsc
			GLfloat sliceVertices[8];
			GLfloat texVertices[8];
                        
			coord &co = norotcoord;
            const int side = 64, buff = 8; // Magic numbers :(
            quadWrite2(texVertices, co.x1,co.y2,co.x2,co.y1);
			quadWrite(sliceVertices, 0,buff,0+side,buff+side);
            float alpha = 1.0-(ticks-rotYellingIconSince)/FPS34;
            
            States(true, false, false); // Can't rot icon-- no fog
            glBindTexture(GL_TEXTURE_2D, atlas);
            jcVertexPointer(2, GL_FLOAT, 0, sliceVertices);
            jcTexCoordPointer(2, GL_FLOAT, 0, texVertices);
            jcColor4f(1,1,1,alpha);
            jcLoadIdentity();

            if (rotYellingIconRot || rotYellingIconDir) {
                jcTranslatef(ztracebounds_orig.width/2, ztracebounds_orig.height/2, 0);
                jcRotatef(rotYellingIconRot*90, 0, 0, 1);
                if (rotYellingIconDir)
                    jcRotatef(180,0,1,0);

                if (0 == rotYellingIconRot%2) // Because the corner isn't in the same place for all rots
                    jcTranslatef(-ztracebounds_orig.width/2, -ztracebounds_orig.height/2, 0);
                else
                    jcTranslatef(-ztracebounds_orig.height/2, -ztracebounds_orig.width/2, 0);
                
            }            
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
		
		rePerspective = true;
		        
        // #### #### ---- Angle edit mode
        if (EAngle == edit_mode) {
            const int button_height = 17;
            const int button_width = 40;
            // DRAW HERE
            for(int c = 0; c < 8; c++) {
                bool filled = false;
                if (level[jumpman_s].dontrot & (1 << c)) {
                    filled = true;
                    Color3f(1.0, 0.0, 0.0);
                } else if (level[jumpman_s].rots & (1 << c)) {
                    filled = true;
                    Color3f(1.0, 1.0, 0);
                } else {
                    jcColor4f(0.5,0.5,0.5,0.5);
                }
                
                jcPushMatrix();
                jcTranslatef(ztracebounds_orig.width/2,ztracebounds_orig.height/2,0);
                jcRotatef(180 + 45*c,0,0,1);
                jcTranslatef(0,button_height*8,0);
                
                States(false, false);
                if (filled) {
                    GLfloat squareVertices[8];
                    quadWrite(squareVertices, -button_width, -button_height, button_width, button_height);
                    jcVertexPointer(2, GL_FLOAT, 0, &squareVertices[0]);
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // POINT SIZE?
                } else {
                    GLfloat squareVertices[8] = {-button_width,-button_height,-button_width,button_height,button_width,button_height,button_width,-button_height
                    };
                    jcVertexPointer(2, GL_FLOAT, 0, &squareVertices[0]);
                    glDrawArrays(GL_LINE_LOOP, 0, 4); // POINT SIZE?
                }
                
                jcPopMatrix();
            }
        }
        
        if (drawingFloaters.size() > 0 && !(edit_mode == ENothing && paused)) {                        
			list<floater>::iterator b = drawingFloaters.begin();
#define TFPS (FPS*3/4)
			while(b != drawingFloaters.end()) {                
				float alpha = 1.0;
                float trot = 0;
				if ((*b).c >= (*b).t-TFPS) {
					alpha = ((double)(*b).t-(*b).c)/TFPS;
				} else if ((*b).c < TFPS) {
					alpha = ((double)(*b).c)/TFPS;
				}
                if (controlRot & CGrav && tilt) { // Got some unit issues here
                    if (!(*b).haveStickyRot) {
                        (*b).stickyRot = lastdrawescat * 90;
                        (*b).haveStickyRot = true;
                    }
                    trot = (*b).stickyRot;
                } else
                    trot = holdrot * 90;                
                States(true, false, false); // Text floaters-- no fog
				jcColor4f((*b).r,(*b).g,(*b).b,alpha);
				jcLoadIdentity();
//				jcTranslatef((*b).x + (*b).dx*(*b).c, (*b).y, 0);
                //		jcTranslatef(chassis->p.x, chassis->p.y, 0);
                if (trot) {
                    jcTranslatef(ztracebounds_orig.width/2, ztracebounds_orig.height/2, 0);
                    jcRotatef(trot, 0, 0, 1);
                    jcTranslatef(-ztracebounds_orig.width/2, -ztracebounds_orig.height/2, 0);
                }
                
                GLERR("PreText");
                (*b).live(); 
                int x = 0, y = 0;
                if (ztracebounds_orig.width > 320) {
                    x = (ztracebounds_orig.width - 320) / 2;
                    y = doingEnding ? (ztracebounds_orig.height - 320) / 2 : 0;
                }
                [(*b).texture drawInRect:CGRectMake(x,y,320,320)];   

                if (!doingEnding) // Ending slowdown has consequences, so handle floater lifetime funny
                    (*b).c++;
                else
                    (*b).c = (endingAt - (*b).born) * REAL_FPS / 44100; // Convert sample ticks to frame ticks

                
                GLERR("PostText");
				
				if ((*b).c > (*b).t) {
					if ((*b).lockout = lockoutHeavy)
						floatheavy = false;
                    (*b).die();
					b = drawingFloaters.erase(b); // Iterates b, incidentally
					resetFloater(); // Might not do anything
				} else {
					b++;
				}
            }
        }
                
        // #### #### ---- Tilt hint (debug)
		if (optAxis) {
            States(0,0);
			jcVertexPointer(2, GL_FLOAT, 0, traceLine);
			drawGauge(0.25);
            
#if GYRO_DEBUG            
            float x = accX, y = accY, z = accZ;
            drawRaw(x, y, 0.6);
            drawRaw(x, z, 0.7);
            drawRaw(y, z, 0.8);
#endif
		}        
	}
	
#if DBGTIME
    static bool everdisplayed = false;
    if (!everdisplayed) {
        printTimeOfDay("d");
        everdisplayed = true;
    }
#endif
    
    // #### #### ---- Update the world
	
	ticks++;
	
	moonBuggy_update();
}

void calculateCoordinates() { // Rename?
    jcMatrixFetch(edit_projection, edit_model_view, edit_viewport);
}

cpVect viewToSpace(cpVect in) {
	GLfloat desiredZ = -1;
	GLfloat x, y, buffZ, tempX, tempY, tempZ;
	
	gluProject(0,0, desiredZ, edit_model_view, edit_projection, edit_viewport, &tempX, &tempY, &buffZ);
	gluUnProject(in.x, ztracebounds_orig.height-in.y, buffZ, edit_model_view, edit_projection, edit_viewport, &x, &y, &tempZ); // FLIPPED

//		ERR("screenToGL x %d y %d -> x %lf, y %lf\n", (int)in.x, (int) in.y, x, y);
	
    return cpvunrotate(cpv(x, y), cpvforangle(roten));	
}

cpVect spaceToView(cpVect in) {
	GLfloat desiredZ = jumpman_s-1;
	GLfloat x, y, tempZ;
		
	gluProject(in.x,in.y, desiredZ, edit_model_view, edit_projection, edit_viewport, &x, &y, &tempZ);
	
//			ERR("glToScreen x %f y %f -> x %lf, y %lf\n", in.x, in.y, x, y);
	
	return cpv(x, ztracebounds_orig.height-y);	
}

void this_side_up() {
#if 1
    holdrot = (holdrot ? 0 : 3);
#else
	holdrot++;
	holdrot %= 4;
#endif

	if (holdrot % 2) {
		ztracebounds.width = ztracebounds_orig.height; ztracebounds.height = ztracebounds_orig.width;
	} else {
		ztracebounds.height = ztracebounds_orig.height; ztracebounds.width = ztracebounds_orig.width;
	}		
}

string scoreKeyFor(LoadedFrom from, string filename) {
    string result;
    switch (from) {
        case FromInternal:
            result = "i:";
            break;
        case FromOwn:
            result = "e:";
            break;
        case FromDown:
            result = "d:";
            break;
        default:
            result = "?:";
            break;
    }
    result += filename;
    return result;
}    

void scoreKeyIs(LoadedFrom from, string filename) {
    currentScoreKey = scoreKeyFor(from, filename);
}

void clearScoreKey(LoadedFrom from, string filename) {
    string scoreKey = scoreKeyFor(from, filename);
    ERR("CLEARING SCORE KEY %s\n", scoreKey.c_str());
    scores.erase(scoreKey);
    skips.erase(scoreKey);
    skips_entry.erase(scoreKey);
    
    dirtyScores = true;
    SaveHighScores(); // I guess I just save this always now 
}

void wload(LoadedFrom from, string filename, bool buildScoreKey, bool andClearEverything) {
    if (andClearEverything)
        clearEverything();
    lastLoadedFrom = from;
	lastLoadedFilename = filename;
	availableLevelsCount = 1;
    
    if (buildScoreKey) {
        scoreKeyIs(from, filename);
        currentSkip = skips.count(currentScoreKey) ? skips[currentScoreKey] : -1;
        cpVect skipxy = skips_entry.count(currentScoreKey) ? skips_entry[currentScoreKey] : cpvzero;
        currentSkipX = skipxy.x;
        currentSkipY = skipxy.y;
        ERR("Will use score key [%s]\n", currentScoreKey.c_str());
        ERR("Current skip %d (%f,%f)\n", currentSkip, currentSkipX, currentSkipY);
    } else 
        ERR("NO SCORE KEY\n");
	
    // Remember: available "levels" count is really an available flags count
    availableLevelsAreFake = currentScoreKey.empty();
	if (!availableLevelsAreFake && scores.count(currentScoreKey))
		availableLevelsCount = scores[currentScoreKey].first.time.size() + 1;
    
    loadGame(from, filename.c_str(), !availableLevelsAreFake ? availableLevelsCount-1 : 0);
    
	if (availableLevelsAreFake || flags.size() < availableLevelsCount)
		availableLevelsCount = flags.size();
	
    chassis->p.x = jumpman_x;
    chassis->p.y = jumpman_y;

    ERR("%d xzzx %f,%f\n", jumpman_s, chassis->p.x, chassis->p.y);
    
#if 0
	pause(false);
	onEsc = WPause;
	startTimer();
	jumpman_reset();
#endif
}

void initFog() {
	int gap = howDeepVisible();
	{
		float FogCol[3]={0.0f,0.0f,0.0f}; // Define a nice light grey
		glFogfv(GL_FOG_COLOR,FogCol);     // Set the fog color
	}
	glEnable(GL_FOG);
	glFogf(GL_FOG_MODE, GL_LINEAR); // Note the 'i' after glFog - the GL_LINEAR constant is an integer.
	glFogf(GL_FOG_START, 1.01);
	glFogf(GL_FOG_END, 1.0f + gap);
}


void mainInit() {
    srandomdev();
    
    InitLang();
    
	cpInitChipmunk();
	moonBuggy_init();
	display_init();
		    
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    login_name = [[prefs objectForKey:@"login_name"] retain];
    if (login_name) {
        login_pass = [[prefs objectForKey:@"login_pass"] retain];
        login_email = [[prefs objectForKey:@"login_email"] retain];
        have_saved_login = true;
    }
    {
        // Alternate iPad defaults-- set here so prefs gets a chance to overwrite it
        if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
            savedControlRot = CKnob;
        }
        
        if ([prefs objectForKey:@"savedControlMove"]) {
            savedControlMove = (ControlModeMove)[prefs integerForKey:@"savedControlMove"];
            holdrot = UI_USER_INTERFACE_IDIOM() != UIUserInterfaceIdiomPad
                && (savedControlMove & CButtonMove) ? 3 : 0;
        }
        if ([prefs objectForKey:@"savedControlRot"])
            savedControlRot = (ControlModeRot)[prefs integerForKey:@"savedControlRot"];
        if ([prefs objectForKey:@"audioMode"])
            audioMode = (AudioMode)[prefs integerForKey:@"audioMode"];
        
        if ([prefs objectForKey:@"optColorblind"])
            optColorblind = [prefs boolForKey:@"optColorblind"];
        
    }
    
    soundInit();

    // Guarantee that the editor and download directories exist
    char filename[FILENAMESIZE]; // duplicated from loadEditorFile. ugh
    char filename2[FILENAMESIZE];
    
    userPath(filename2); // What is this towers of hanoi nonsense
    
    snprintf(filename, FILENAMESIZE, "%s/%s", filename2, DIRPATH);    
    mkdir(filename, 0777);

    snprintf(filename, FILENAMESIZE, "%s/%s", filename2, DOWNPATH);    
    mkdir(filename, 0777);    
    
    // Init music (as opposed to sound effects)
    
    musicDelegate = [[MusicDelegate alloc] init];
    kickMusic();
}

// ------------- END C TYPE STUFF, BEGIN OBJECTIVEC --------------------

#include "musicnames.h"

AVAudioPlayer *localPlayer = nil;
#if OKAY_SHUFFLE
MPMusicPlayerController *shufflePlayer = nil;
#endif

int musicfile_last = -1, musicfile_last_last = -1;

void killMusic() {
    if (localPlayer) {
        [localPlayer release];
        localPlayer = nil;
    }
#if OKAY_SHUFFLE
    if (shufflePlayer) {
        [shufflePlayer stop];
        [shufflePlayer release];
        shufflePlayer = nil;
    }
#endif
}

void kickMusic() {
    ARR("KICK IT\n");
    if (!musicDelegate)
        return; // Just to be paranoid.
    
    ARR("KICK IT! %d\n", audioMode);
    
    switch (audioMode) {
        case LOCAL_AUDIO: {
            NSError *temp = nil;
            char filename[FILENAMESIZE];
            
            int musicfile_new;
            long r;
            do { r = random(); musicfile_new = r % MUSICFILE_COUNT; } while (musicfile_last == musicfile_new || musicfile_last_last == musicfile_new);
            musicfile_last_last = musicfile_last;
            musicfile_last = musicfile_new;
            
            internalPath(filename, internalMusic[musicfile_new]);
            localPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL: 
                           [NSURL fileURLWithPath:toNs(filename) isDirectory:NO] error: &temp];
            localPlayer.delegate = musicDelegate;
            ARR("Music %p, err %d, r %ld (%d), playing %s\n", localPlayer, temp?[temp code]:0, r, musicfile_new, internalMusic[musicfile_new]);
            
//            addFloater(internalMusic[musicfile_new]); // For debugging only; doesn't work in practice b/c first kick call is before game starts
        } break;
#if OKAY_SHUFFLE
        case SHUFFLE_AUDIO: {
            shufflePlayer = [[MPMusicPlayerController applicationMusicPlayer] retain];
            shufflePlayer.shuffleMode = MPMusicShuffleModeSongs;
            [shufflePlayer setQueueWithQuery: [MPMediaQuery songsQuery]];
        } break;
#endif
    }    
}

void wantMusic(bool music) {
    if (edit_mode != ENothing) // No music in edit mode!
        return;
    
    ARR("WANTMUSIC %s\n", music?"Y":"N");
    
    if (music) {
        if (audioIsHalted) { // Just in case-- should be covered by wakingnow
            AudioResume();
        }
            
        switch (audioMode) {
            case LOCAL_AUDIO: {
                if (!localPlayer)
                    kickMusic();
                BOOL success;
                int tries = 0;
                do {
                    success = [localPlayer play];
                    tries++;
                    ARR("PLAY %s\n", success?"SUCCESS":"FAIL");
#if SELF_EDIT
                    if (!success)
                        sbell.reset();
#endif
                } while (!success && tries < 5); // Why not. TODO: Move down to like 2?
            } break;
#if OKAY_SHUFFLE
            case SHUFFLE_AUDIO: {
                if (!shufflePlayer)
                    kickMusic();
                [shufflePlayer play];
            } break;
#endif
        }
    } else {
        switch (audioMode) {
            case LOCAL_AUDIO: {
                [localPlayer pause];
            } break;
#if OKAY_SHUFFLE
            case SHUFFLE_AUDIO: {
                [shufflePlayer pause];
            } break;
#endif
        }        
    }
}

void PhoneSleepingNow() {
    PhonePausingNow();
    fastAsleep = true;
    if (audioMode == SHUFFLE_AUDIO) {
        AudioHalt();
    }        
    if (audioMode == LOCAL_AUDIO && localPlayer)
        [localPlayer stop];
}

void PhoneWakingNow() {
    ARR("WAKE\n");
    fastAsleep = false;
    
    if (audioMode == SHUFFLE_AUDIO) {
        AudioResume();
    } else if (audioMode == LOCAL_AUDIO && localPlayer) {
        [localPlayer prepareToPlay];
    }
}

@implementation MusicDelegate

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag {
    ARR("FINISHED successful: %s\n", flag?"Y":"N");
    killMusic();
    kickMusic();
    [localPlayer play];
}
- (void)audioPlayerDecodeErrorDidOccur:(AVAudioPlayer *)player error:(NSError *)error {
    ARR("DECODING ERROR? %d\n", [error code]);
    killMusic();
    kickMusic();
    [localPlayer play];
}

@end

float accX = 0, accY = 0, accZ = 0;

@implementation AccelSlurper
- (void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration {
	accX = [acceleration x];
	accY = [acceleration y];
	accZ = [acceleration z];
    
    if (tilt)
        tilt->run();
}
@end

@interface ThreadShell : NSObject { // LATER
}
@end

// A class extension to declare private methods
@interface EAGLView ()

@property (nonatomic, retain) EAGLContext *context;
@property (nonatomic, assign) NSTimer *animationTimer;

- (BOOL) createFramebuffer;
- (void) destroyFramebuffer;

@end

@implementation EAGLView

@synthesize context;
@synthesize animationTimer;
@synthesize animationInterval;


// You must implement this method
+ (Class)layerClass {
    return [CAEAGLLayer class];
}

EAGLContext *getContext(EAGLRenderingAPI api, EAGLSharegroup *sharegroup) {
    if (!sharegroup)
        return [[EAGLContext alloc] initWithAPI:api];
    else
        return [[EAGLContext alloc] initWithAPI:api sharegroup: sharegroup];    
}

- (id)initBasic: (EAGLSharegroup *)sharegroup {
    off = false;
    
    // Get the layer
    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
    
    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithBool:optSplatter?YES:NO], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
    
    GLERR("lay");
    
    context = getContext(kEAGLRenderingAPIOpenGLES2, sharegroup);
    if (context) {
        es2 = true;
        ERR("ES2\n");
    } else {
        es2 = false;
        ERR("ES1\n");
        context = getContext(kEAGLRenderingAPIOpenGLES1, sharegroup);
    }
    
    if (!context || ![EAGLContext setCurrentContext:context]) {
        GLERR("con1");
        [self release];
        ERR("DIED\n");
        return nil;
    }
    GLERR("con2");
    
    animationInterval = 1.0 / 500; // 1/60?
    
    return self;
}

- (void)bury {
    [EAGLContext setCurrentContext:context];
    [self destroyFramebuffer];
    [self stopAnimation];
    off = true;
    screentraces_clear();
}

//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder {    
    if ((self = [super initWithCoder:coder])) {
        if ((self = [self initBasic:nil])) {
            // Yay it worked
        }
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame {    
    if ((self = [super initWithFrame:frame])) {
        if ((self = [self initBasic:nil])) {
            // Yay it worked
        }
    }
    return self;
}

- (id)initWith:(EAGLView *)source {    
    if ((self = [super initWithFrame:[source frame]])) {
        if ((self = [self initBasic: source.context.sharegroup])) {
            // Yay it worked
        }
    }
    return self;
}

- (void)layoutSubviews {
    if (off) return;
    [EAGLContext setCurrentContext:context];
    [self destroyFramebuffer];
    [self createFramebuffer];
    [self drawView];
}

- (BOOL)createFramebuffer {
    
    glGenFramebuffersOES(1, &viewFramebuffer);
    glGenRenderbuffersOES(1, &viewRenderbuffer);
    
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
    [context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(CAEAGLLayer*)self.layer];
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);
    
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
    
    if (USE_DEPTH_BUFFER) {
        glGenRenderbuffersOES(1, &depthRenderbuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
        glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
    }
    
    if(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
        NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
        return NO;
    }
    
    return YES;
}


- (void)destroyFramebuffer {
    
    glDeleteFramebuffersOES(1, &viewFramebuffer);
    viewFramebuffer = 0;
    glDeleteRenderbuffersOES(1, &viewRenderbuffer);
    viewRenderbuffer = 0;
    
    if(depthRenderbuffer) {
        glDeleteRenderbuffersOES(1, &depthRenderbuffer);
        depthRenderbuffer = 0;
    }
}


- (void)startAnimation {
    self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:animationInterval target:self selector:@selector(drawView) userInfo:nil repeats:YES];
}


- (void)stopAnimation {
    self.animationTimer = nil;
}


- (void)setAnimationTimer:(NSTimer *)newTimer {
    [animationTimer invalidate];
    animationTimer = newTimer;
}


- (void)setAnimationInterval:(NSTimeInterval)interval {
    
    animationInterval = interval;
    if (animationTimer) {
        [self stopAnimation];
        [self startAnimation];
    }
}


- (void)dealloc {
    
    [self stopAnimation];
    
    if ([EAGLContext currentContext] == context) {
        [EAGLContext setCurrentContext:nil];
    }
    
    [context release];  
    [super dealloc];
    
    ERR("GL OUT\n");
}

- (void)drawView {
}

@end

@implementation JumpView

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (id)initBasic:(EAGLSharegroup*)sharegroup {
    printTimeOfDay("s");
    haveInitialized = false;
    
    if ((self = [super initBasic:sharegroup])) {  
        [self setMultipleTouchEnabled:YES];

        // super initwithbasic should have left us in "our" context, so...
        if (!basicInit) {
            basicInit = true;

                CGSize tempbounds = [self bounds].size;
            ztracebounds_orig = tempbounds;
            ztracebounds = ztracebounds_orig;
            aspect = ztracebounds.height / ztracebounds.width; // TODO
            
            ERR("BOUNDS %f %f\n", ztracebounds.height, ztracebounds.width);
            
            //		viewport[0] = 0; viewport[1] = 0; viewport[2] = backingWidth; viewport[3] = backingHeight;
            
            mainInit(); // What if self is nil? 
                        
            slurper = [[AccelSlurper alloc] init];
            UIAccelerometer *acc = [UIAccelerometer sharedAccelerometer];
            [acc setUpdateInterval: 1/80.0];
            [acc setDelegate: slurper];
            
                    printTimeOfDay("i");
            
#if 0
            // Originally I had the main thread go to the bother of waiting for the worker thread to wake up before continuing
            // Except, why bother? And it was slowing stuff down awful.
            need_halt = true; // THREAD
            worker_alive = true;
            needed.lock();
            [NSThread detachNewThreadSelector:@selector(workerThread:) toTarget: [[[ThreadShell alloc] init] autorelease] withObject:nil];
            while (need_halt)
                needed.wait();
            needed.unlock();
#endif

            const char *filename = "Main.jmp";
            
#if OKTHREAD
            lastLoadedFrom = FromInternal;
            lastLoadedFilename = filename;
            THREADERR("should b\n");
            need_load = true;
            worker_alive = true;

            [NSThread detachNewThreadSelector:@selector(workerThread:) toTarget: [[[ThreadShell alloc] init] autorelease] withObject:nil];

//            needed.set(need_load, true);
#else
            LoadHighScores();
            if (LoadHibernate()) {
                switchScreen(escScreen, kCATransitionPush, kCATransitionFromBottom, 0);
                intro = false;
            } else {
                wload(FromInternal, filename, true);
            }
#endif        
        }
    }        
    
    return self;
}

- (void)drawView {
    if (off) return;
    
    if (completelyHalted) {
        if (!haltMessage.empty()) {
            UIAlertView *someError = [[UIAlertView alloc] initWithTitle: _(@"Everything broke") message: toNs(haltMessage) delegate: nil cancelButtonTitle: nil otherButtonTitles: nil];
            [someError show];
            [someError release]; // Redundant, this will never close?
            haltMessage.clear();
        }
        
        return;
    }
    
    [EAGLContext setCurrentContext:context];
    
	normalFb = viewFramebuffer; // We can do this later
	normalWidth = backingWidth; normalHeight = backingHeight;
	
	if (!haveInitialized) {
		haveInitialized = true;
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);        
        if (!es2) {
            initFog(); 
            glPointSize(3.0);
        } else {
            es2Basic(true);
            jcMatrixInit();
        }
	}
	
#if TAP_DEBUG
//    ERR("--- DISPLAY ---\n");
#endif
    
	if (controlMove != CButtonMove) {
        bool found_any_wraith = false;
        if (controlMove & CMoveAuto && ticks - lastWraithAt > 2) { // Remember this next bit only counts when we're in one particular mode-- and NOT editing                
            // Sometimes the event queue gets crowded and we lose a touchesEnded event.
            // We need to clear these out or they will make touchArrows do terrible things.
            // We bother only with things that were swipes before they were lost.
            // If a finger is very still it is normal to not receive events about it.
            bool found_wraith = false;
            do {
                UITouch * w = NULL;
                found_wraith = false;
                
                for(hash_map<UITouch *, touchinfo>::iterator b = allTouches.begin(); b != allTouches.end(); b++) {
                    touchinfo &t = (*b).second;
                    if (t.isFresh) {
                        t.isFresh = false;
                    } else {
                        if (fabs(t.v.x) > SWIPESWITCH_AT || fabs(t.v.y) > SWIPESWITCH_AT) {
                            ERR("WRAITH TOUCH %f\t%f\n", t.v.x, t.v.y);
                            w = (*b).first;
                            found_wraith = true;
                            found_any_wraith = true;
                            break;
                        }
                    }
                }
                if (found_wraith) {
                    if (w == lastJumpAt) { // Clear out that last jump before we go
                        CGPoint where = [w locationInView: self];
                        screentraces_push_front(cpv(where.x, ztracebounds_orig.height-where.y), t_jump);
                        lastJumpAt = NULL;
                    } // Note: Duplicated from popAll. And we DON'T give twist esc a chance to trigger
                    
                    allTouches.erase(w);
                }
            } while (found_wraith);
            
            lastWraithAt = ticks;
        }
        
		if (autostill || found_any_wraith)
			[self touchArrows];
		else
			[self drawArrows:false];	
	}
	
	if (controlMove == CButtonMove) {
		if (holdrot) {	// Seriously just ugly
			if (input_power < -0.5)
				buttonFlare[3] = ticks;
			else if (input_power > 0.5)
				buttonFlare[2] = ticks;
		} else {
			if (input_power < -0.5)
				buttonFlare[2] = ticks;
			else if (input_power > 0.5)
				buttonFlare[3] = ticks;
		}
	}    
    
	display();
    
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer); // For some reason I used to do this twice. Why?
    [context presentRenderbuffer:GL_RENDERBUFFER_OES];
}

// START CONTROLS

// Creating a touch
- (void) pushAll:(NSSet *)touches {
	NSEnumerator *enumerator = [touches objectEnumerator];
	UITouch *value;
	while ((value = [enumerator nextObject])) {
		touchinfo &t = allTouches[value];
		cpVect loc = cpv([value locationInView: self]);
		
        t.isFresh = true;
        
		if (t.count == 0 && (escHave && !(controlRot == CGrav && (wantrot || rotstep)))
            && [touches count] == 1 && allTouches.size() == 1) { // FIXME: What if allTouches gets "stuck"?
			cpVect unEscAt = escAt; unEscAt.y = ztracebounds_orig.height - unEscAt.y - 1;
			cpFloat triDist = cpvlength(cpvsub(loc, unEscAt));
			if (triDist < 50) {
                if (!(controlRot & CKnob)) {
                    PhonePausingNow();
                    
                    t.poisoned = true;
                } else {
                    t.wasEscAttempt = true;
                }
			}
		}
		
		if (edit_mode != ENothing) {
			cpVect ploc = viewToSpace(loc);
            
			if (t.count == 0) {				
				cpVect chassisPixel = spaceToView(chassis->p);
				cpFloat dist = cpvlength(cpvsub(loc, chassisPixel));
                
                //				ERR("%f,%f vs %f, %f = %f\n", loc.x, loc.y, chassisPixel.x, chassisPixel.y, dist);
                //				ERR("%f,%f vs %f, %f = %f\n", ploc.x, ploc.y, chassis->p.x, chassis->p.y, cpvlength(cpvsub(ploc, chassis->p)));
				if (paused || jumpman_unpinned || dist > 50) { // Paused or dead or too far away
					t.editPoisoned = true;
					t.v = ploc;
				}
			}
            //			ERR("%x: count %d poisoned? %s\n", &t, t.count, t.editPoisoned?"Y":"N");
		}
		
		t.push( loc );
	}
}

// Destroying a touch
- (void) popAll:(NSSet *)touches {
	NSEnumerator *enumerator = [touches objectEnumerator];
	UITouch *value;
	while ((value = [enumerator nextObject])) {
#if OKAY_TRACER
		if (value == lastJumpAt) { // Clear out that last jump before we go
			CGPoint where = [value locationInView: self];
			screentraces_push_front(cpv(where.x, ztracebounds_orig.height-where.y), t_jump);
			lastJumpAt = NULL;
		} else if (controlRot & CKnob) {
            touchinfo &t = allTouches[value];
            if (t.wasEscAttempt && !t.poisoned) { // Note: These are many fewer restrictions than we had on the initial "was esc"
                cpVect loc = cpv([value locationInView: self]); // Code duplication
                cpVect unEscAt = escAt; unEscAt.y = ztracebounds_orig.height - unEscAt.y - 1;
                cpFloat triDist = cpvlength(cpvsub(loc, unEscAt));
                
                if (triDist < 50) {
                    PhonePausingNow();
                        
                    t.poisoned = true; // LITTLE LATE TO WORRY ABOUT THIS NOW
                }
            }
        }
#endif
		
		allTouches.erase(value);
	}
}

// Handles the start of a touch
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
//    ERR("B %d\t%d\n", [touches count], [event.allTouches count]);
    
	[self pushAll:touches];
	
	[self touchArrows];
}

// Handles the continuation of a touch.
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{  
#if TAP_DEBUG
    ERR("M %d\t%d\n", [touches count], [event.allTouches count]);
#endif
    
	[self pushAll:touches];
	
	// Calculate v (velocity)
	for(hash_map<UITouch *, touchinfo>::iterator b = allTouches.begin(); b != allTouches.end(); b++) {
		if ((*b).second.editPoisoned) continue;
		
		cpVect &accum = (*b).second.v;
		accum = cpvzero;
//		(*b).second.vu = 0;
        
#if GESTURE_DEBUG
		ERR("%x, %d:\t", (*b).first, (*b).second.count);
#endif
		
		cpVect &end = (*b).second.touches[ (*b).second.top ];
		
        int acount = 0;
        
		for(int c = 1; c < (*b).second.count && c < TOUCHAVG; c++) { // Touchiter
			cpVect &rec = (*b).second.touches[ ((*b).second.top + TOUCHTRACK - c) % TOUCHTRACK ];
			cpVect r2 =  cpvmult( cpvsub( rec, end ) , 1.0/c );
			
#if GESTURE_DEBUG
			ERR("%d=[%.1f,.1%f]\t", c, r2.x, r2.y);
#endif
			
			accum = cpvadd(accum, r2);
            
            acount++;
		}
		
		if (acount) {
			accum = cpvmult(accum, 1.0/acount);
		}
		
#if GESTURE_DEBUG
		ERR("== [%.1f,%.1f]", accum.x, accum.y);
#endif
        
        (*b).second.a = fmod(cpvtoangle(accum) + holdrot*(M_PI/2) - (controlRot & CGrav ? (tilt->center()+1) * M_PI : 0), 2*M_PI);
        if ((*b).second.a < -M_PI) (*b).second.a += 2*M_PI;
        if ((*b).second.a >  M_PI) (*b).second.a -= 2*M_PI;
        
        
#if GESTURE_DEBUG
		ERR(" @%.1f\n", (*b).second.a/M_PI*180);
#endif
	}
	
	[self touchArrows];
	
#define KNOB_DEBUG 0
    
	if (controlRot & CKnob) {
		for(hash_map<UITouch *, touchinfo>::iterator b = allTouches.begin(); b != allTouches.end(); b++) {
#if !KNOB_DEBUG
			if ((*b).second.poisoned)
				continue;
#endif
			hash_map<UITouch *, touchinfo>::iterator c = b; c++; // There has got to be a better way to do this
			for(; c != allTouches.end(); c++) {
				if ((*c).second.poisoned)
					continue;
				
				bool rotted = false;
				cpVect tb = (*b).second.touches[(*b).second.top];
				cpVect tc = (*c).second.touches[(*c).second.top];
				cpVect vb = (*b).second.v;
				cpVect vc = (*c).second.v;
				cpVect m = cpvmult(cpvadd(tb,tc),0.5);
				
				cpFloat tob = cpvtoangle(cpvsub(tb,m));
				cpFloat tob2a = cpvtoangle(cpvsub(cpvadd(tb,vb),m));
				cpFloat tobDiff = tob-tob2a;
                
                if (tobDiff < M_PI)
                    tobDiff += 2*M_PI;
                if (tobDiff > M_PI)
                    tobDiff -= 2*M_PI;
				
                //				cpFloat toc = cpvtoangle(cpvsub(tb,m));
                //				cpFloat toc2a = cpvtoangle(cpvsub(cpvadd(tb,vb),m));
                //				cpFloat tocDiff = toc-toc2a;
				
                //				cpFloat cross = cpvcross(vb, vc);
				cpFloat dot = cpvdot(vb, vc);

#if KNOB_DEBUG
                ERR("DOT %f\tTOB %f%s\n", dot, tobDiff, dot<-400?(tobDiff<0?"\tR":"\tL"):"");
#endif
                
				if (dot < -400
#if KNOB_DEBUG
                    && !(*c).second.poisoned
#endif
                    && !doingEnding
                    ) {
					rotted = true;
					if (rotkey(tobDiff < 0 ? ROTL : ROTR)) {
						(*b).second.poisoned = true;
						(*c).second.poisoned = true;
#if OKAY_TRACER
						screentraces_push_front(cpv(m.x, ztracebounds.height-m.y), t_rot);
#endif
					}
				}
				
				//ERR("%x vs %x: cross %f, dot %f, tobd %f, tocd %f%s\n", (*b).first, (*c).first, cross, dot, tobDiff, tocDiff, rotted?" !!!!!":"" );
				if (rotted)
					break;
			}				
		}	
	}
	
	if (MODE_JUMPS(controlMove)) {
		for(hash_map<UITouch *, touchinfo>::iterator b = allTouches.begin(); b != allTouches.end(); b++) {
			if ((*b).second.poisoned || (*b).second.editPoisoned) continue; 
			
			cpVect v = screenToViewV((*b).second.v);
			
            if (!jumping) {
                bool nowjump = false;
                
                nowjump = v.y > 25;

#if NEW_VELOCITY                
                float lest = -20000;
                cpVect &end = (*b).second.touches[ (*b).second.top ];
                for(int c = 1; c < (*b).second.count && !nowjump; c++) { // Touchiter
                    int at = ((*b).second.top + TOUCHTRACK - c) % TOUCHTRACK;
                    int when = (*b).second.touchWhen[ at ];
                    if (ticks-when > TOUCHTICKS)
                        break;
                    cpVect &rec = (*b).second.touches[ at ];
                    cpVect r2 =  cpvsub( rec, end );
                    
                    if (controlRot & CGrav)
                        r2 = cpvunrotate(r2, cpvforangle( (tilt->center()+1)*M_PI )); // WHY +1
                    
                    if (r2.y > lest)
                        lest = r2.y;
                    if (r2.y > 50)
                        nowjump = true;
                }            
#endif
                if (nowjump) {
                    jump();
                    if (jumping)
                        lastJumpAt = (*b).first;
                    break;
                }
			}
                //		printf("%x of %d: x %f, y %f MOVED x %f, y %f\n", value, [touches count], where.x, where.y, from.x-where.x, from.y-where.y);
		}	
	}
}

// Handles the end of a touch event when the touch is a tap.
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
#if TAP_DEBUG
    ERR("E %d\t%d\n", [touches count], [event.allTouches count]);
#endif
    
#if GESTURE_DEBUG
	ERR("\n");
#endif
    
	if (DOES_TOUCHES(edit_mode)) {
		touchinfo *edt;
		int edc = 0;
		for(hash_map<UITouch *, touchinfo>::iterator b = allTouches.begin(); b != allTouches.end(); b++) {
			if ((*b).second.poisoned) continue;
			if ((*b).second.editPoisoned) { // TODO: Only do this if member of touches
				if (edc < 1) edt = &(*b).second;
				edc++;
			}
		}
		if (1 == edc && !edt->dragPoisoned)
			clickLastResortHandle(edt->touches[edt->top], 1, false, edt->v);
	}	
	
	[self popAll: touches];
	
	[self touchArrows];
}

// Handles the end of a touch event.
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
#if TAP_DEBUG
    ERR("C %d\t%d\n", [touches count], [event.allTouches count]);
#endif
    
	[self popAll:touches];
	
	[self touchArrows];
}

// We have updated the contents of the allTouches hash. Now convert that into movement instructions
- (void)touchArrows
{
	CGRect bounds = [self bounds];
	bool l = false, r = false;
	float lastv = 0;
    
	for(hash_map<UITouch *, touchinfo>::iterator b = allTouches.begin(); b != allTouches.end(); b++) {
		touchinfo &t = (*b).second;
		
		cpVect top = screenToView(t.touches[t.top]);
        
//        ERR("Ticks %d\tv %f,%f\n", ticks, t.v.x, t.v.y);
		
        if (controlMove == CFollow && jumpman_s < level.size()) {
            cpVect p = spaceToView(chassis->p);
            cpVect d = cpvsub(top, p);
            cpVect r = cpvmult( d, 1/rFor(level[jumpman_s]) );
            
#define JUMPTHRESH 27
#define RUNTHRESH  18
            
            if (r.x > RUNTHRESH)
                t.dir = t_right;
            else if (r.x < -RUNTHRESH)
                t.dir = t_left;
            else
                t.dir = t_center;
            
//                        ERR("%f,%f\t%f,%f\t%f,%f\t%f,%f (%f..%d)\n", p.x,p.y,top.x,top.y,d.x,d.y,r.x,r.y, rFor(level[jumpman_s]), (int)t.dir);
            
            if (r.y < -JUMPTHRESH && !jumping) {
                jump();
                if (jumping)
					lastJumpAt = (*b).first;
            }
        }
        
		if ((t.isNew || t.poisonException) && (controlMove == CButtonMove || controlRot == CButtonRot)) {
			int pane;
			bool onLeft;
			bool onRight; // flip
            
			if (holdrot) { // THIS IS EXTREMELY WORRYING. I HAD TO SWAP WIDTH AND HEIGHT ON HOLDROT AND I DON'T KNOW WHY
				pane = (ztracebounds.width-top.y-1) / controlButtonWidth;
				onLeft = top.x < controlButtonWidth;
				onRight = top.x > ztracebounds.height-controlButtonWidth; // flip
			} else { // THIS IS BROKEN OUTRIGHT
				pane = (ztracebounds.height-top.y-1) / controlButtonWidth;
				onLeft = (ztracebounds.width-top.x-1) < controlButtonWidth;
				onRight = (ztracebounds.width-top.x-1) > ztracebounds.width-controlButtonWidth; // flip
			}				

            // Panes: 0 -> Rot; 1 -> Run; 2 -> Jump
            // Poison exceptions: 0 -> No exception; 1 -> Fired but have never run; 2 -> Just running; 3 -> Ran, then fired
            // (Why is poisonException named this?)
			if (pane < 3) { // If valid pane					
				if ((onLeft || onRight) && 
					((controlMove == CButtonMove && pane>0) || (controlRot==CButtonRot && pane==0))) {
					bool passthrough = false; // Passthrough means this has already triggered and we're not interested
					
					t.poisoned = true;
                    
                    // The goal following is to make it possible to "swerve" from button to button
                    
					if (pane != 1) { // If a non-run button
                        if (!t.poisonException) // Never fired before
                            t.poisonException = 1;
                        else if (t.poisonException == 1 || t.poisonException == 3) // If already fired
							passthrough = true;
						else // Poison exception is 2; previously was just running
							t.poisonException = 3;
					}
					
					if (!passthrough) { // Handle display ramifications of not passing through
						if (onLeft)
							buttonFlare[pane*2+1] = ticks;
						if (onRight)
							buttonFlare[pane*2] = ticks;
					}
					
					if (!holdrot) { // Paired with something even uglier elsewhere
						bool t = onLeft;
						onLeft = onRight;
						onRight = t;
					}					
					
					if (1 == pane || t.poisonException > 1) { // If running now, or previously hit run on this touch
						if (onRight)
							t.dir = t_right;
						else if (onLeft)
							t.dir = t_left;
					}
					if (!passthrough) switch (pane) {
						case 0:
							if (onLeft) rotkey(ROTR);
							if (onRight) rotkey(ROTL);
							break;
						case 1:
							t.poisonException = 2;
							break;
						case 2:
							jump();
							break;
					}
				}
				
				t.isNew = false;
			}
		}
		
		// "case CSwipe" "case CEditMove"
		if (controlMove & (CSwipe | CEditMove | CMoveAuto) && !(t.poisoned || t.editPoisoned)) {
			cpVect v = screenToViewV(t.v);
            
#if 0
			lastv = v.x / -7.5;
#else // DO_OVERFLOW
			lastv = v.x / -5;
#endif
//            t.vu++;
//            if (t.vu > 1)
//                ERR("\tVU! %d\n", t.vu);
            
			// Let CMoveAutos in, but kick them out here if we haven't "started moving"
			// (Or if we were a CSwipe before, but we've stopped moving now)
			if (controlMove & CMoveAuto) {
//                printf("%f < %f < %f\n", RAD(45), t.a, RAD(135));
                
                if (t.a > RAD(45) && t.a < RAD(135)) { // If they're generally drifting upward, don't count it as a swipe
                    t.pushPoisonedAt = ticks;
                }                
                if (v.y > 10) { // If they even THINK of moving up, don't count it as a push
                    t.swipePoisonedAt = ticks;
                }
                
				if (!(controlMove & CSwipe) && ticks-t.pushPoisonedAt > POISONTICKS) { // Auto, and when we entered we were pushing
					if (fabs(v.x) > SWIPESWITCH_AT)
						switchauto(CSwipe);
				} else {	// Auto, and when we entered we were swiping
					if (fabs(v.x) > SWIPESWITCH_AT || ticks-t.swipePoisonedAt<POISONTICKS) { // swiping OR old swipe still in play
						autostill = false;
                        // To switch to push, a never-swiped touch must hit the push region and there must
                        // be no swiping for PUSHSWITCH_AFTER ticks.
					} else {
						float line = ztracebounds.width/2; // Tragic code duplication
						float d = ztracebounds.width*DEADZONE;
						if (top.x < line - d || top.x > line + d) { // If in the "red range"
							
							if (!autostill)
								autostillsince = ticks;
                            //							printf("STILLING %d\n", ticks - autostillsince);
							if (ticks - autostillsince > PUSHSWITCH_AFTER) {
                                if (!(controlMove & CPush)) { // Maybe this should be deferred until we can actually draw a circle?
                                    switchauto(CPush);
                                }
							} else
								autostill = true;
						}
					}
				}
			}			
			
			if (controlMove & (CSwipe | CEditMove)) {
				if (v.x < -SWIPESWITCH_AT) {
					t.dir = t_right;
					t.swipePoisonedAt = ticks;
				} else if (v.x > SWIPESWITCH_AT) {
					t.dir = t_left;
					t.swipePoisonedAt = ticks;
				}
				
				if (t.dir == t_right) {
					l = false;
					r = true;
				} else if (t.dir == t_left) {
					l = true;
					r = false;
				}
			}
		}
		
		// "case CPush"
		if(controlMove & CPush && !t.poisoned) {
			float line = ztracebounds.width/2; // flip
			float d = ztracebounds.width*DEADZONE;
			
			t.dir = t_center;
			if (top.x < line - d) {
				l = true;
				t.dir = t_left;
			} else if (top.x > line + d) {
				r = true;
				t.dir = t_right;
			}
		}
		
		// "case CButtonMove"
		if (controlMove & (CButtonMove | CFollow)) {
			if (t.dir == t_right) {
				r = true;
			} else if (t.dir == t_left) {
				l = true;
			}
		}
	}	
	newarrows(l,r);
	if (controlMove & (CSwipe | CEditMove)) // I guess this overrides newarrows?
		input_power = lastv * input_power_modifier;
	
	if (controlMove != CButtonMove)
		[self drawArrows:true];
}

cpVect rescanMod(cpVect v) { // REMOVE ME
	return v;
}

// We have converted the allTouches hash into movement instructions. Now convert those into drawing instructions
- (void)drawArrows:(bool)movement
{
    if (paused && edit_mode == ENothing)
        return;
    
	CGRect bounds = [self bounds];
	touchinfo *edt[2];
	int edc = 0;
	
	for(hash_map<UITouch *, touchinfo>::iterator b = allTouches.begin(); b != allTouches.end(); b++) {
		if ((*b).second.poisoned) continue;
		if ((*b).second.editPoisoned) {
			if (edc < 2) edt[edc] = &(*b).second;
			edc++;
			continue;
		}

#if OKAY_TRACER
		UITouch *value = (*b).first;
		CGPoint where = [value locationInView: self];
		
		trace_type t = t_null;
		
		if (jumpman_unpinned) {
			t = t_null;
		} else if (value == lastJumpAt) {
			t = t_jump;
			lastJumpAt = NULL;
		} else if ((*b).second.dir == t_left) {
			t = input_power_last_facing < 0 ? t_move : t_nullmove;
		} else if ((*b).second.dir == t_right) {
			t = input_power_last_facing > 0 ? t_move : t_nullmove;
		}
        
        if ( !(ticks - (*b).second.drewAt > 2 || t == t_jump || t == t_rot
              || cpvlengthsq((*b).second.v) > 500 ))
            continue;
//        ERR("%f\n", cpvlengthsq((*b).second.v));
        (*b).second.drewAt = ticks;
		
        bool huge = t == t_move && nextMoveHuge;
        if (huge) nextMoveHuge = false;
        
		if (t == t_move && (controlMove & (CSwipe | CEditMove)))
			t = t_swipe;        
		
		screentraces_push_front(cpv(where.x, bounds.size.height-where.y), t, huge);
#endif
	}		
    
	if (1 == edc && !edt[0]->dragPoisoned && DOES_TOUCHES(edit_mode)) { // Previously this checked EWalls. Isn't that a true noop?
		clickLastResortHandle(edt[0]->touches[edt[0]->top], 1, true, edt[0]->v);
	}
	
	if (movement && 2 == edc && edt[0]->count > 1 && edt[1]->count > 1) {
		edt[0]->dragPoisoned = true; edt[1]->dragPoisoned = true;
		cpVect &t0 = edt[0]->touches[ edt[0]->top ], &lt0 = edt[0]->touches[ (edt[0]->top + TOUCHTRACK - 1) % TOUCHTRACK ],
        &t1 = edt[1]->touches[ edt[1]->top ], &lt1 = edt[1]->touches[ (edt[1]->top + TOUCHTRACK - 1) % TOUCHTRACK ];
		cpVect mt = cpvmult(cpvadd(t0,t1),0.5), lmt = cpvmult(cpvadd(lt0,lt1),0.5);
		cpFloat dt = cpvlength(cpvsub(t0, t1)), ldt = cpvlength(cpvsub(lt0, lt1)); 
		
        //		ERR("%f,%f becomes %f,%f\n", t0.x, t0.y, lt0.x, lt0.y);
		
        //		ERR("%f,%f => %f,%f; %f / %f = %f\n", mt.x,mt.y,lmt.x,lmt.y, dt, ldt, ldt/dt);
		
        if (edit_mode == EWalls)
            scan = cpvadd(scan, rescanMod( cpvsub(viewToSpace(mt), viewToSpace(lmt)) ));
#if !FOG_DEBUG
		scan_r *= (dt/ldt);
#else
        extern float fogf;
        fogf *= (dt/ldt);
        States(false, true);
        jcFogFactor(fogf);
        
        States(true, true);
        jcFogFactor(fogf);
        
        for(int c = 0; c < 5; c++) {
            GLfloat tempX, tempY, tempZ;
            gluProject(0,0, -c, edit_model_view, edit_projection, edit_viewport, &tempX, &tempY, &tempZ);
            ERR("%d: %f\n", -c, tempZ);
        }
        ERR("\n");
#endif
        
        tryZoomFeedback(); // May do nothing
	}
}

// END CONTROLS

@end

#if OKTHREAD

// External state
condition needed;   // work needed
condition completed;     // work done
bool worker_alive = false;
bool need_halt = false; // Set when need to put worker to sleep, clear when done.
bool need_detach = false;
bool need_load = false; // Set when need worker to read xml, clear when done.
bool need_some_load = false; // Set when worker to load some level, clear when done.
int some_load_prioritize = -1; // Set when worker to load some level; worker sets to -1 after reading

// Internal state
bool worker_halted = false;
bool some_load_dirty = false;
int some_load_at = 0;

void halt_worker() {
    needed.lock();
    THREADERR("tbg want halt\n");
    if (!worker_halted) {
        THREADERR("tbg demanding halt\n");
        need_halt = true; // THREAD
        while (need_halt)
            needed.wait();
    }
    needed.unlock();    
}

@implementation ThreadShell

- (void) workerThread: discard {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    needed.lock();
    while(1) {
        bool keep_working = false;
        
        if (need_detach) { 
            THREADERR("tbg a\n");
            needed.unlock();
            completed.set(need_detach, false);
            [pool release];
            return;
        }
        
        if (need_load) { 
            THREADERR("tbg b\n");
            needed.unlock(); keep_working = true;
            
                    printTimeOfDay("t");
            
            LoadHighScores();
            if (LoadHibernate()) {
                loadedHibernate = true;
            } else {
                wload(lastLoadedFrom, lastLoadedFilename, true);
            }
            printTimeOfDay("v");

            completed.set(need_load, false);
            
            needed.lock();
        }
        
        if (need_some_load) {
            if (some_load_prioritize >= 0) {
                THREADERR("tbg will b %d\n", some_load_prioritize);
                some_load_at = some_load_prioritize;
                some_load_dirty = true;
                some_load_prioritize = -1;
            }
            THREADERR("tbg b (%d %d)\n", some_load_at, level.size());
            needed.unlock(); keep_working = true;
            
            if (some_load_at < level.size()) {
                level[some_load_at].realForceLoad();
                THREADERR("tbg did load %d\n", some_load_at);
                some_load_at++;
            }
            if (some_load_at >= level.size()) {
                if (some_load_dirty) {
                    for(some_load_at = 0; some_load_at < level.size(); some_load_at++) {
                        if(!level[some_load_at].loaded)
                            break;
                    }
                    if (some_load_at >= level.size())
                        some_load_dirty = false;
                }
                if (!some_load_dirty)
                    completed.set(need_some_load, false);
            }
            
            needed.lock();
        }
        
        if (need_halt) { // This one's special; we "reset" and go directly to sleep after.
            THREADERR("tbg d\n");
            need_halt = false;
            need_detach = false;
            need_load = false;
            need_some_load = false;
            some_load_prioritize = -1;
            keep_working = false;
            needed.broadcast();
        }
        if (!keep_working) {
            THREADERR("tbg halted\n");
            
            [pool release];
            pool = [[NSAutoreleasePool alloc] init];
            
            worker_halted = true;
            needed.wait();
            worker_halted = false;
            THREADERR("tbg awake\n");
        }
    }
}

@end

#endif

// --- MORE C TYPE STUFF WHICH I JUST WANT OUT OF THE WAY ---

void BombBox(string why) {
	NSLog(@"SUPPOSED-TO-BE-FATAL ERROR: %s\n", why.c_str());
    if (bombExcept) {
        throw runtime_error(why);
    } else {
        completelyHalted = true;
        haltMessage = why;
    }
}

void FileBombBox(string filename) {
	string because;
	if (filename.empty())
		because = _("An internal file could not be opened.");
	else
		because = _("Could not open file:\n");
	BombBox(because + filename + "\n");
}

// --- END MORE C TYPE STUFF ---

// --- TILT STUFF ----

// FLICK

#define DEF_CENTER 0.0
#define BASE_AVG 100
#define VAL_AVG 5
#define LINE_SPAN 0.08
#define SAFE_SPAN 0.05

#define VEL_AVG 5
#define DSAFE_SPAN 0.05
// Could go over 0.3?
#define DLINE_SPAN 0.2

averager::averager(int _count) : accum(DEF_CENTER), maxcount(_count) {
    window.push(DEF_CENTER);
}

void averager::push(float v) {
    accum += v;
    window.push(v);
    while (window.size() > maxcount) {
        accum -= window.front();
        window.pop();
    }
}
float averager::at() {
    return accum/window.size();
}

struct flick_tilter : public tilter {
    averager base, val;
    list<float> vel;
    
    // Dubious utility
    float top;
    float d;
    float foundSafe[2];
    float foundLine[2];
    char lastRottedKey; // Internal state
    
    virtual float rawAt();
    virtual float center();
    virtual float at();
    virtual float line(bool high);
    virtual float safe(bool high);
    virtual float shadow(bool high);
    
    virtual bool enabled();
    virtual void run();
    virtual void reset();
    
    flick_tilter();
};

flick_tilter::flick_tilter() : base(BASE_AVG), val(VAL_AVG), vel(VEL_AVG), lastRottedKey(0) {
    top = DEF_CENTER;
    key = 0;
    foundSafe[0] = 1000; foundSafe[1] = -1000;
    foundLine[0] = 1000; foundLine[1] = -1000;
}

void flick_tilter::reset() {
    vel.clear();
    key = 0;
    lastRottedKey = 0;
    foundSafe[0] = 1000; foundSafe[1] = -1000;
    foundLine[0] = 1000; foundLine[1] = -1000;
}

bool flick_tilter::enabled() {
    float _x = accX, _y = accY, _z = accZ;
    
    _x = sqrt(_x*_x+_y*_y); _y = 0;
    
    float a1 = 0 == _x && 0 == _z ? M_PI : atan2(_x,_z);
    a1 = fabs(a1-M_PI/2); // Goes into range 0..pi, "flat" is near 0 and pi
    return (a1 < M_PI/3); // Why /3?
}

float flick_tilter::rawAt() { return top; }
float flick_tilter::center() { return base.at(); }
float flick_tilter::at() { return val.at(); }
float flick_tilter::line(bool high) { // This is somewhat silly
    return foundLine[high?1:0];
}
float flick_tilter::safe(bool high) {
    return foundSafe[high?1:0];
}

float flick_tilter::shadow(bool high) {
#if 0
    int c = high?1:0;
    const float csafe[2] = {tilt.safe(0), tilt.safe(1)};
    const float cline[2] = {tilt.line(0), tilt.line(1)};
    
    const float ldist = (c==0 ? top-cline[0] : cline[1]-top);
    const float span = (c==0 ? csafe[0]-cline[0] : cline[1]-csafe[1]);
    const float alpha0 = (ldist > 0 ? (span-ldist)/span : 1)/2;
    return alpha0;
#endif
//    ERR("%s%s|\td %.3f s %.3f\n", high?"1":"2", (d<0 ^ high)?"x":" ", d, DLINE_SPAN);
    if (lastRottedKey == (high?ROTR:ROTL)) {
        float a1 = at();
        float csafe = safe(high);
        float cline = line(high);
        if (high) { // Instead of littering with ?:, let's try this.
            a1 = -a1;
            csafe = -csafe;
            cline = -cline;
        }

        if (a1 < cline)
            return 1;
        if (a1 > csafe)
            return 0;
        return 1-(a1-cline)/(csafe-cline);
    } else {
        float fd = fabs(d);
        if (fd < DSAFE_SPAN || (d<0 ^ high))
            return 0;
        return (fd-DSAFE_SPAN)/DLINE_SPAN;
    }
}

void flick_tilter::run() { // Note: Lacks the advantage of lin_int, gets called only at frames. Maybe try update() next time?
//        float cline[2] = {line(0), line(1)};
        //float csafe[2] = {safe(0), safe(1)};
    
    { // update()
        float x = accX, y = accY, z = accZ;
        switch(holdrot) { // Needed: Actual 3D rotation					
            case 1: case 3: default: x = accX; y = accY; break;
            case 2: x = -accY; y = accX; // Wrong? Why does this work?
                //				case 1: case 3: x = -accX; y = -accY; // Wrong? Why doesn't this work?
            case 0: x = accY; y = -accX;
        }			        
        z = sqrt(x*x+z*z); x = 0;
        top = 0 == y && 0 == z ? -M_PI/2 : atan2(y,z);
//        top -= M_PI/2; // Redundant?
        top /= (M_PI/2);
    }
    
//    if (top > cline[0] && top < cline[1]) // TODO
//        base.push(top);
    
    val.push(top);
    
    float a1 = at();
    vel.push_back(a1);
    while (vel.size() >= VEL_AVG) vel.pop_front();
    
    d = 0; // Basically duping "accum" trick in touchesMoved
    float newsafe = 0;
    for(list<float>::iterator b = vel.begin(); b != vel.end(); b++) {
        float rec = *b;
        float r2 = top-rec;
        if (fabs(r2) > fabs(d)) { // ...only different.
            d = r2; 
            newsafe = rec;
        }
    }
    
    // Act
    if (!lastRottedKey && enabled()) {
        if (d > DLINE_SPAN) {
            key = ROTR;
            lastRottedKey = ROTR;
            foundSafe[1] = newsafe;
            foundLine[1] = a1;
        } else if (d < -DLINE_SPAN) {
            key = ROTL;
            lastRottedKey = ROTL;
            foundSafe[0] = newsafe;
            foundLine[0] = a1;
        }
    } else if (a1 > safe(0) || a1 < safe(1)) {
        lastRottedKey = 0;
        foundSafe[0] = 1000;
        foundSafe[1] = -1000;
        foundLine[0] = 1000;
        foundLine[1] = -1000;
    }
}

// GRAVITY

struct grav_tilter : public tilter {
    float x,y,z;
    grav_tilter();
    
    virtual bool enabled();
    virtual void run();
    virtual void reset();
    virtual float rawAt();
    virtual float at();
    virtual float center();
    virtual float safe(bool);
    virtual float line(bool);
};

grav_tilter::grav_tilter() {
    key = 0;
    reset();
}

#define AVG(a, b) ((a*(avgBy-1) + b)/avgBy)

void grav_tilter::run() {
    const int avgBy = 3;
    
    x = AVG(x, accX); y = AVG(y,accY); z = AVG(z,accZ);
}

void grav_tilter::reset() {
    x = 0; y = 0; z = 0;
}

bool grav_tilter::enabled() {
    float _x = x, _y = y, _z = z;
    
    _x = sqrt(_x*_x+_y*_y); _y = 0;
    
    float a1 = 0 == _x && 0 == _z ? M_PI : atan2(_x,_z);
    a1 = fabs(a1-M_PI/2); // Goes into range 0..pi, "flat" is near 0 and pi
    return (a1 < M_PI/2-3*M_PI/32);
}

float grav_tilter::rawAt() {
    float a1 = 0 == accX && 0 == accY ? M_PI : atan2(accX,accY);
    return a1 / M_PI;
}

float grav_tilter::at() {
    float a1 = 0 == x && 0 == y ? M_PI : atan2(x,y);
    return a1 / M_PI;
}

float grav_tilter::center() {
    float r;
    if (level[rotting_s].camera == cam_fixed) {
        r = level[rotting_s].staticBody->a / M_PI;
    } else {
        r = - roten / M_PI;
    }    
    
    r += 1; // Without this angles come out "upside down"

    r = fmod(r, 2); // Roten has a span of 4π
    if (r < -1) r += 2;
    else if (r > 1) r -= 2;

    return r;
}

float grav_tilter::safe(bool) { // This one doesn't use safe(), but this lets me graph the center value in debug
    return center();
}

#define ROTDIFFMARGIN 0.125

// Notice this is extremely inefficient for what is being done
float grav_tilter::line(bool dir) {
    int distance = 0;
    
    if (level[rotting_s].rots == 0 || level[rotting_s].rots == 1) {
        distance = 1;
    } else {
        unsigned char rots = level[rotting_s].rots | level[rotting_s].dontrot;
        do {
            if (dir)
                rotl(rots);
            else
                rotr(rots);
            distance++;
        } while (!(rots & 1));
    }
        
    return center() + ROTDIFFMARGIN * (dir?1:-1) * distance * 1.1; // 1.1 for leeway
}

// GENERAL

tilter *tilt = NULL;

ControlModeRot lastControlRot = CRotDisabled;

void adjustControlRot() {
    if (lastControlRot != controlRot) {
        if (tilt) {
            delete tilt; tilt = NULL;
        }
        
        if (controlRot & CGrav) {
            tilt = new grav_tilter;
        } else if (controlRot & CTilt) {
            tilt = new flick_tilter;
        }
        
        lastControlRot = controlRot;
    }
}


// --- END TILT STUFF

// -- ENDING

bool haveDoneBlue = false;

// Duplicates load_texture. Yeah, whatever
void construct_mountain() {
    GLuint &result = mountain;
    
    if (mountain) {
        // Should like "assert" or something here
        return; 
    }
    
    escScreen = AFTER_ENDING_MAIN_SCREEN; // THIS IS AN ENTIRELY INAPPROPRIATE PLACE TO DO THIS. WOULD GOBLUE BE BETTER?
    
    char filename[FILENAMESIZE];
	internalPath(filename, "kyou_hill_texture.png");
	ERR("OK? %s\n", filename);
	slice *thing = new slice();
	thing->construct(filename, false); // Doesn't REALLY construct
	
	{
		int width = thing->width;
		int height = thing->height;
		int xo = 0;
		int yo = 0;
		
		GLuint *textureData = new GLuint[width*height];
		memset(textureData, 0, sizeof(GLuint)*width*height);
		for(int x = 0; x < thing->width; x++) {
			for(int y = 0; y < thing->height; y++) {
				unsigned int color = htonl(thing->pixel[thing->width-x-1][thing->height-y-1]);
                const int off = ((x+xo)+(y+yo)*width);
				textureData[off] = color;
			}
		}
		
		glGenTextures(1, &result);
		
		glBindTexture(GL_TEXTURE_2D, result);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);				
        
        delete[] textureData;
	}
	
	delete thing;   
}

void destroy_mountain() {
    glDeleteTextures(1, &mountain);
    mountain = 0;
}

void goBlue() {
	if (endingAtMode > 0) {
		if (!haveDoneBlue) {
			glClearColor(0.0, 1.0, 1.0, 1.0);
			haveDoneBlue = true;
		}
	} else
		haveDoneBlue = false;
}

// -- END ENDING

// --- INTRO

#define INTRO_CHARS 9
GLuint intro_texture = 0;
int intro_char = 0;
const int intro_press_at[INTRO_CHARS+2] = {0, 155, 235, 286, 546, 611, 741, 871, 1027, 1274, 1774};
int cursor_ticks = 0;

int intro_press(int x) {
    return (100 + intro_press_at[x])/TPF;
}

void intro_draw(int ox, int oy, int ix, int iy, int w, int h) {
    GLfloat sliceVertices[8];
    GLfloat texVertices[8];
    const int scale = 3;
    const double tscale = 1.0/128;
    const int px = 3, py = 2;

    quadWrite(texVertices, tscale*ix,tscale*iy, tscale*(ix+w),tscale*(iy+h));
    quadWrite(sliceVertices, scale*(px+ox),ztracebounds_orig.height-scale*(py+oy),scale*(px+ox+w),ztracebounds_orig.height-scale*(py+oy+h));

    jcVertexPointer(2, GL_FLOAT, 0, sliceVertices);
    jcTexCoordPointer(2, GL_FLOAT, 0, texVertices);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);		    
}

void display_intro() {
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, normalFb); // Shouldn't have to do this every time, should I?
    glViewport(0, 0, normalWidth, normalHeight);
    jcMatrixMode(GL_PROJECTION);
    jcLoadIdentity();
    jcOrtho( 0, ztracebounds_orig.width, 0, ztracebounds_orig.height, -1, 1);
    jcMatrixMode(GL_MODELVIEW); 
    jcLoadIdentity();
    
    if (0 == ticks) {
        printTimeOfDay("o");
        intro_texture = load_texture("runhello_sq.png");
        printTimeOfDay("p");
    }
    
    States(true, false, false); // Texture, Color // Opening text -- no fog
    glBindTexture(GL_TEXTURE_2D, intro_texture);
            
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    jcColor4f(1,1,1,1);      // Color Gray?
    
    int at = (min(intro_char, INTRO_CHARS)+1)*7;
    intro_draw(0, 0, 0, 0, at, 9);
    if (int(ticks/(FPS/8))%2) {
        if (intro_char < INTRO_CHARS)
            intro_draw(at, 0, 10*7, 0, 7, 9); // Cursor at 10th position
        else
            intro_draw(0, 8, 10*7, 0, 7, 9);
    }

    ticks++;
    cursor_ticks++;
    
    if (ticks > intro_press(intro_char) && !(intro_char > INTRO_CHARS+1)) { // kludge to let us run off end
        intro_char++;
//        cursor_ticks++; // FIXME: Why do I do this twice?
    }
    
    if (intro_char > INTRO_CHARS+1
#if OKTHREAD
            && !need_load
#endif
        ) {
        intro = false;
        
#if FOG
        extern float fogf;
        States(false, true);
        jcFogFactor(fogf);
        
        States(true, true);
        jcFogFactor(fogf);        
#endif
        
        if (loadedHibernate) 
            switchScreen(escScreen, kCATransitionPush, kCATransitionFromTop, 0.2);
        else
            switchScreen(MAIN_SCREEN, kCATransitionPush, kCATransitionFromTop, 0.2);
    }
}

// --- END INTROa

const char * deviceName() {
    return toString([[UIDevice currentDevice] model]);
}