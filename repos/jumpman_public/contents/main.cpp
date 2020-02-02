// DISPLAY LOGIC

/* Copyright (c) 2007 Scott Lembcke
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
  
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "kludge.h"

#include "chipmunk.h"
#include "lodepng.h"
#include "sound.h"
#include "internalfile.h"
#include "FTGLTextureFont.h"
#include <queue>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "tinyxml.h"
#include "jumpman.h"
#include "color.h"

#ifdef WINDOWS

// Why on earth is this necessary?
#define htonl not_htonl
#define ntohl not_htonl

inline long not_htonl (long i) { // Mingw has failed me but windows only ever has one endianness anyway.
	long r;
    char *p = (char *)&r;
	char *c = (char *)&i;
	    p[0] = c[3];
        p[1] = c[2];
        p[2] = c[1];
        p[3] = c[0];
	return *((long *)p);
}
#endif

#define SLEEP_TICKS 16
#define DRAW_DEBUG 0
#define AXIS_DEBUG 0
#define RAINBOW_DEBUG 0
#define ZERO_DEBUG 0
#define AEOLIST_DEBUG 0
#define LOG_AUDIO 0
#define IN_A_WINDOW 0
#define NO_AUDIO 0
#define FOG_EXPERIMENTAL 0

FTFont* floating = NULL;
GLuint ftex;

enum lockoutTypes {
	lockoutNone = 0,
	lockoutHeavy,
	lockoutLayer,
	lockoutRot,
	lockoutDebug
};

struct floater {
	int t, c;
	float x, y, dx, r, g, b;
	lockoutTypes lockout;
	string text;
	floater(string _text = string(), int _t = 0, int _c = 0, float _x = 0, float _y = 0, float _dx = 0, lockoutTypes _lockout = lockoutNone, float _r = 1, float _g = 1, float _b = 1) {
		text = _text;
		t = _t; c = _c;
		x = _x; y = _y; dx = _dx;
		r = _r; g = _g; b = _b;
		lockout = _lockout;
	}
};

list<floater> drawingFloaters;
queue<string> pendingFloaters;

int floatcount = 0;
float floatHeight = 0;
bool floatheavy = false;

enum ControlType { ct_none, ct_key, ct_pad, ct_dpad, ct_hat };
ControlType controlType[NUMCONTROLS];
SDLKey controlKeys[NUMCONTROLS];
int controlPad[NUMCONTROLS];
double aspect = 1;
double surplusSpin = 0;
cpVect rescan;
unsigned int mountainl = NULL;
int lastRebornAt = -60;
bool doInvincible = false, doingInvincible = false;
extern bool exiting_entire_game;

#define COUNT_SAVEDOPTIONS 4
bool savedOptions[COUNT_SAVEDOPTIONS];
bool optSplatter = false, &optColorblind = savedOptions[0], &optAngle = savedOptions[1], &optSlow = savedOptions[2], &optWindow = savedOptions[3];

#if defined(__APPLE__) && !IN_A_WINDOW
// Support for alt-tab-equals-minimize feature
bool minimizeWant = false, minimizeAlreadyPaused = false;
#endif
bool controlsScreenAlreadyPaused = false;

unsigned int tempdl[2] = {0,0};

int fullscreenw = 0, fullscreenh = 0;
int surfacew = 0, surfaceh = 0;
std::vector<unsigned char> screenshotimage;
LodePNG::Encoder screenshotencoder;

#define MAXAXIS 256
int controlAxes[MAXAXIS] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // Do not back up; consider removing. This joystick support code is way more bloated than it should be.
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int controlHat[MAXAXIS] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // Do not back up; consider removing. This joystick support code is way more bloated than it should be.
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define DPAD_THRESHOLD 16380
inline int hatStatus(int hat, int value) { return ((hat&0xFF)<<8)|(value&0xFF); }
inline int hatHat(int status) { return ((status>>8)&0xFF); }
inline bool hatMatchHat(int status, int hat) { return hatHat(status) == hat; }
inline int hatValue(int status) { return (status&0xFF); }

bool gotJoystick = false;

cpSpace *uiSpace;
GLdouble originOrthX, originOrthY, oneOrthX, oneOrthY;
GLdouble originPerX, originPerY, onePerX, onePerY;
double button_width = 180, button_height = 72;

// WTF?
static float texture[] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                           1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                           0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                           0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};


int lastSdlTime = 0, sdlTime = 0;

// Geez, look at how much state I wind up accumulating the instant I go outside the fuzzy confines of chipmunk!!
// This is all *JUST* to handle the dying/exit animations!
int jumpman_s = 0, jumpman_d = 0, jumpman_l = 0; float jumpman_x = 0, jumpman_y = 0, jumpman_r = 1, scan_x = 0, scan_y = 0, scan_r = 1;
bool rePerspective = false;
bool jumpman_unpinned = false;
jumpman_state jumpmanstate = (jumpman_state)0;
pan_type pantype = (pan_type)0;
cpVect panf, pant; float panfz, pantz, panfr, pantr; int pantt, panct, jumptt, jumpct;

bool drawHelp = true, drawGrid = false, drawPalette = true, drawMph = false, drawControls; 

list<string> toolHelp;

// Global framecount clock
int ticks = 0;

sqtone sjump(5000, 100, 0.05);
sqtone sland(5000, 400, 0.05);
sqtone sball(5000, 200, 0.05);
sqtone sbell(30000, 200, 0.05);
notone splodey(30000, 300, 0.1);
notone shatter(30000, 300, 0.05);
notone slick(500, 1, 0.075);
bool doingEnding = false;
int endingAt = 0, endingAtBar = 0, endingAtMode = 0, endingAtMsg = 0;
extern int modeStartsAt, modeEndsAt;


FILE *audiolog = NULL;

vector<spaceinfo> level;
vector<int> flags;

bool doingControlsScreen = false, doingHiddenControlsScreen = false;
ContainerBase *hiddenControlsScreen[2];
cpSpace *hiddenControlsSpace;
bool controlsScreenProcess(SDL_Event &event);
void destroyControlsScreen();
inline cpSpace *workingUiSpace() { return doingHiddenControlsScreen ? hiddenControlsSpace : uiSpace; }

string dotJmp(string name) {
	return name + ".jmp";
}

string srcFromLevel(TiXmlNode *_element) {
	TiXmlElement *element = (TiXmlElement *)_element;
	TiXmlNode *i = NULL;
	while(1) {
		i = element->IterateChildren("File", i);
		if (!i)
			return "";
		if (i->Type() == TiXmlNode::ELEMENT)
			return ((TiXmlElement *)i)->Attribute("src");
	}
}

string nameFromLevel(TiXmlNode *_element) {
	TiXmlElement *element = (TiXmlElement *)_element;
	const char *explicitName = element->Attribute("name");
	if (explicitName) 
		return explicitName;
	string src = srcFromLevel(element);
	if (!src.empty())
		return src;
	return "[???]";
}

void clearEverything() {
	// Clear out the drawing area
//	cpSpaceRemoveBody(level[jumpman_s].space, chassis);
//	cpSpaceRemoveShape(level[jumpman_s].space, chassisShape);
	//cpSpaceDestroy(level[jumpman_s].space);
	for(int c = 0; c < level.size(); c++)
		cpSpaceFree(level[c].space);
	level.clear();
	flags.clear();
	jumpman_x = 0; jumpman_y = 0; jumpman_s = 0; jumpman_d = 0; jumpman_flag = 0; jumpman_l = 0;
	
	roten = 0;
	wantrot = 0; rotstep = 0; // If we're rotating, stop it.
	rescan = cpvzero;
	surplusSpin = 0;
	
	desiredEnding = 0;
	onWin = WMainMenu; // Maybe a bit paranoid to do this every time?
	
	doInvincible = false; doingInvincible = false;
	
	pantype = pan_dont; // In case they quit in the middle of a zoom or something I guess
	jumpmanstate = jumpman_normal;
	jumpman_unpinned = true;
	
	anglingSince = 0;
}

/* --- ACTUAL PROGRAM --- */

inline void Color3f(double r, double g, double b) {
#if RAINBOW_DEBUG
	glColor3f((double)random()/RANDOM_MAX, (double)random()/RANDOM_MAX, (double)random()/RANDOM_MAX);
#else
	glColor3f(r, g, b);
#endif
}

inline void BlindColorTransform(double &r, double &g, double &b, unsigned int layers) {
	r /= 3; g /= 3; b /= 3;
	if (chassisShape->layers & layers) {
		r += 2/3.0; g += 2/3.0; b += 2/3.0;
	}
}	

inline void BlindColor3f(double r, double g, double b, unsigned int layers) {
	if (optColorblind)
		BlindColorTransform(r, g, b, layers);
	Color3f(r, g, b);
}

double crandr, crandg, crandb;
int lastcrandattick = 0;
#define FADEFACTOR 3.0
inline void lavaColor() {
	if (ticks != lastcrandattick) {
//	if (ticks >= lastcrandattick+5) { // Even this hurts people's eyes, so why bother
		lastcrandattick = ticks;
		crandr = crandr*((FADEFACTOR-1)/FADEFACTOR) + (double)random()/RANDOM_MAX/FADEFACTOR;
		crandg = crandg*((FADEFACTOR-1)/FADEFACTOR) + (double)random()/RANDOM_MAX/FADEFACTOR;
		crandb = crandb*((FADEFACTOR-1)/FADEFACTOR) + (double)random()/RANDOM_MAX/FADEFACTOR;
	}
	  Color3f(crandr, crandb, crandg);
}

bool BackOut() {
	bool didOnEsc = false;
	if (doingHiddenControlsScreen) {
		destroyControlsScreen();
		return true;
	}
	if (WNothing == onEsc)
		Quit();
	else {
		wantClearUi = true;
		want = onEsc;
		didOnEsc = onEsc != WIgnore;
	}
	serviceInterface();
	return didOnEsc;
}

void Quit(int code) {
    SDL_Quit();
	
	{
		char optfile[FILENAMESIZE];
		internalPath(optfile, "controls.obj");
		ofstream f;
		f.open(optfile, ios_base::out | ios_base::binary | ios_base::trunc);
		if (!f.fail()) {
			f.write((char *)controlType, sizeof(controlType));
			f.write((char *)controlKeys, sizeof(controlKeys));
			f.write((char *)controlPad, sizeof(controlPad));
			f.write((char *)savedOptions, sizeof(savedOptions));
		}
	}
	
	SaveHighScores();
	
    exit(code);	
}

void SaveHighScores() {
	char optfile[FILENAMESIZE] = "jumpman.sav";

	ofstream f;
	unsigned int temp;
	f.open(optfile, ios_base::out | ios_base::binary | ios_base::trunc);
ERR("ZZZZ File? %d\n", scores.size());
	if (!f.fail()) {
		temp = htonl(scores.size()); f.write((char *)&temp, sizeof(temp));
		for(hash_map<string, pair<scoreinfo, scoreinfo> >::iterator b = scores.begin(); b != scores.end(); b++) {
			int namelen = (*b).first.size();
			if (namelen > FILENAMESIZE)
				namelen = FILENAMESIZE;
			temp = htonl(namelen); f.write((char *)&temp, sizeof(temp));
			f.write((*b).first.c_str(), namelen);
			
			int listlen = (*b).second.first.time.size();
	ERR("ZZZZ name %s listlen out %d\n", (*b).first.c_str(), listlen);
			temp = htonl(listlen); f.write((char *)&temp, sizeof(temp));
			for(int c = 0; c < listlen; c++) {
				temp = htonl( (*b).second.first.time[c] ); f.write((char *)&temp, sizeof(temp));
				temp = htonl( (*b).second.first.deaths[c] ); f.write((char *)&temp, sizeof(temp));
				temp = htonl( (*b).second.second.time[c] ); f.write((char *)&temp, sizeof(temp));
				temp = htonl( (*b).second.second.deaths[c] ); f.write((char *)&temp, sizeof(temp));
			}
		}	
ERR("ZZZZ File! %d\n", scores.size());
	}
}

void audio_callback(void *userdata, uint8_t *stream, int len) {
	int16_t *samples = (int16_t *)stream;
	int slen = len / sizeof(int16_t);
//	int wavelength = random() % 500; // For debug
	for(int c = 0; c < slen; c++) {
		double value = 0;
		
		if (doingEnding)
			endingTick();
		
		sjump.to(value);
		sland.to(value);
		sbell.to(value);
		sball.to(value);
		splodey.to(value);
		shatter.to(value);
		slick.to(value);
#if NO_AUDIO
		value = 0;
#endif
		samples[c] = value*SHRT_MAX;
	}
	if (audiolog) {
		fwrite(stream, sizeof(short), slen, audiolog);
	}
}

void goOrtho() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-750.0f, 750.0f, -750.0, 750.0, -1.0, 5.0);
	glScalef(2.0*aspect, 2.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
}

void goPerspective() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, 1/aspect, 0.5, 5);
	glScalef(jumpman_r/750.0f, jumpman_r/750.0f, 1.0);
	glMatrixMode(GL_MODELVIEW);
}

float centerOff(const char *str) {
		float llx, lly, llz, urx, ury, urz;
		floating->BBox(str, llx, lly, llz, urx, ury, urz);
		return (urx - llx)/2;
}

void resetFloater() {
	if (pendingFloaters.size() > 0 && (drawingFloaters.size() == 0 || !floatheavy)) {
		int side = floatcount % 2?-1:1;
		drawingFloaters.push_back( floater(
			pendingFloaters.front(),
			FPS*4,
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
	pendingFloaters.push(newFloater);
	resetFloater(); // Will do nothing if there's already a heavy floater
}

void layerYell() {
	char filename[FILENAMESIZE];
	snprintf(filename, FILENAMESIZE, "Layer %d", editLayer);				

	double arr = level[jumpman_s].r[editLayer], gee = level[jumpman_s].g[editLayer], bee = level[jumpman_s].b[editLayer];
	if (optColorblind)
		BlindColorTransform(arr, gee, bee, 0xFF);

	list<floater>::iterator b = drawingFloaters.begin();
	while(b != drawingFloaters.end()) {
		if ((*b).lockout == lockoutLayer)
			b = drawingFloaters.erase(b);
		else
			b++;
	}

	drawingFloaters.push_back( floater(
		filename,
		FPS,
		0,
		- centerOff(filename),
		0,
		0,
		lockoutLayer,
		arr,
		gee,
		bee) );
}

void endingYell(const char *msg, double x, double y, int multiplier) {
	x *= 750.0/2;
	y *= 750.0/2;
	drawingFloaters.push_back( floater(
		msg,
		FPS*4*multiplier,
		0,
		x - centerOff(msg)/2,
		y - floatHeight/2,
		-0.25,
		lockoutNone,
		0,
		0,
		0) );

}

int rotYellingSince = 0, anglingSince = 0;

void rotYell(bool atall, bool fogged) { // Is this even a good idea?
	string no = "Can't rotate"; // And what the hell should it say? Should I go with an icon?
	if (!atall)
		no += " that way";
	
	list<floater>::iterator b = drawingFloaters.begin();
	while(b != drawingFloaters.end()) {
		if ((*b).lockout == lockoutRot)
			b = drawingFloaters.erase(b);
		else
			b++;
	}

	drawingFloaters.push_back( floater(
		no,
		FPS,
		0,
		- (280.0 / aspect + centerOff(no.c_str())/(optAngle? 1 : 2)),
		- (button_height * 4),
		0,
		lockoutRot) );
	
	if (fogged)	
		rotYellingSince = ticks;
}

void debugYell(string text) { // These should really be condensed.
	list<floater>::iterator b = drawingFloaters.begin();
	while(b != drawingFloaters.end()) {
		if ((*b).lockout == lockoutDebug)
			b = drawingFloaters.erase(b);
		else
			b++;
	}

	drawingFloaters.push_back( floater(
		text,
		FPS,
		0,
		- (280.0 / aspect + centerOff(text.c_str())),
		- (button_height * 4),
		0,
		lockoutDebug) );
}

void clicked(cpShape *shape, void *data)
{
	if (!shape->data || ((ControlBase *)shape->data)->bg || ((ControlBase *)shape->data)->text.size()) // Clicks only make sense for things with backgrounds.
		clickConnected = true;
	if (shape->data)
		((ControlBase *)shape->data)->click();
}

void wheeled(cpShape *shape, void *data)
{
	clickConnected = true;
	if (shape->data)
		((ControlBase *)shape->data)->wheel((int)data);
}

void screenshot() {
	time_t rawtime;
	struct tm * timeinfo;
	char filename2[FILENAMESIZE];
	
	mkdir("Screenshots"
#ifndef WINDOWS
				, 0777
#endif
				);
	
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	strftime(filename2, FILENAMESIZE, "Screenshots/%m-%d-%Y %H.%M.%S.png", timeinfo);
	ERR("SAVETO: %s\n", filename2);
				
	screenshotimage.resize(surfacew * surfaceh * 4);

	glReadPixels(0, 0, surfacew, surfaceh, GL_RGBA, GL_UNSIGNED_BYTE, &screenshotimage[0]);

	uint32_t *data = (uint32_t *)&screenshotimage[0];

	for(unsigned y = 0; y < surfaceh/2; y++)
	for(unsigned x = 0; x < surfacew; x++)
	{ 
		unsigned y2 = surfaceh-y-1; 
		unsigned int temp;
		temp = data[y * surfacew + x];
		data[y * surfacew + x] = data[y2 * surfacew + x] | htonl(C_META);
		data[y2 * surfacew + x] = temp | htonl(C_META);
	}
	
	LodePNG::Encoder screenshotencoder;
	std::vector<unsigned char> screenshotbuffer;
	screenshotencoder.encode(screenshotbuffer, screenshotimage, surfacew, surfaceh);

	LodePNG::saveFile(screenshotbuffer, filename2);

}

void sdl_surface_init(bool in_a_window)
{
	SDL_Surface *surface;
#if IN_A_WINDOW
	in_a_window = true;
#endif
	if (in_a_window)
		surface = SDL_SetVideoMode(
			800, 600, 0,
			SDL_OPENGL);
	else
		surface = SDL_SetVideoMode(
				fullscreenw, fullscreenh, 0, SDL_FULLSCREEN | 
				SDL_OPENGL);
			
  if (surface  == NULL ) {
    REALERR("Unable to create OpenGL screen: %s\n", SDL_GetError());
	Quit(2);
  }
  
  aspect = double(surface->h)/surface->w;
  surfacew = surface->w; surfaceh = surface->h;
  
  ERR("Window size %dx%d ratio %lf\n", surface->w, surface->h, aspect);
  
  #if VSYNC_DESPERATION
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
  #endif
  SDL_WM_SetCaption("Jumpman", NULL);
}

bool haveDoneBlue = false;

void construct_mountain() {
	special_ending_floor_construction = true;
	char filename[FILENAMESIZE];
	slice *mountain = new slice();
	mountainl = glGenLists(1);
	
	internalPath(filename, "kyou_hill.png");

	mountain->construct(filename, true); // Does funny construct b/c doingEnding
	double scale = 4.8;
	
	glNewList(mountainl,GL_COMPILE);

	glTranslatef(-mountain->width*scale/2, mountain->height*scale/2, 0);
		
	glBegin(GL_QUADS); // Duplication?
	for(vector<block>::iterator b = mountain->blocks.begin(); b != mountain->blocks.end(); b++) {
			uint32_t color = ntohl(b->color);
		   unsigned char *rgb = (unsigned char *)&color;
		   glColor3f(rgb[0]/255.0, rgb[1]/255.0, rgb[2]/255.0);
			
		   glVertex2f(b->x*scale, b->y*-scale);
		   glVertex2f(b->x*scale, (b->y + b->height)*-scale);
		   glVertex2f((b->x + b->width)*scale, (b->y + b->height)*-scale);
		   glVertex2f((b->x + b->width)*scale, b->y*-scale);
	}
	glEnd();
	glEndList();

	delete mountain;
	special_ending_floor_construction = false;
}

void initGL();
void recreate_surface(bool in_a_window) {
	ERR("BACK FROM OUTER SPACE");
	sdl_surface_init(in_a_window);
	initGL();
	for(hash_map<unsigned int, plateinfo *>::iterator b = pinfo.begin(); b != pinfo.end(); b++)
		if ((*b).second)
			(*b).second->reconstruct();
	construct_mountain();
	haveDoneBlue = false;
}

void sdl_init()
{	
	{ // Load controls and options first, because they are relevant to full screen or no
		bool failure = false;
		char optfile[FILENAMESIZE];
		internalPath(optfile, "controls.obj");

		ifstream f;
		f.open(optfile, ios_base::in | ios_base::binary);
		if (!f.fail()) {
			f.seekg (0, ios::end);
			if (0 != f.tellg()) {
				f.seekg (0, ios::beg);
				f.read((char *)controlType, sizeof(controlType));
				f.read((char *)controlKeys, sizeof(controlKeys));
				f.read((char *)controlPad, sizeof(controlPad));
				f.read((char *)savedOptions, sizeof(savedOptions));
			} else failure = true;
		} else failure = true;
		if (failure) {		
			// OKAY DEFAULT CONTROLS
			controlKeys[LEFT] = SDLK_LEFT;
			controlKeys[RIGHT] = SDLK_RIGHT;
			controlKeys[JUMP] = (SDLKey)' ';
			controlKeys[ROTR] = (SDLKey)'a';
			controlKeys[ROTL] = (SDLKey)'d';
			for(int c = 0; c < NUMCONTROLS; c++ )
				controlType[c] = ct_key;
		}
		controlType[QUIT] = ct_key;
		controlKeys[QUIT] = (SDLKey)SDLK_ESCAPE;
		controlType[CONTROLS] = ct_key;
		controlKeys[CONTROLS] = (SDLKey)SDLK_F1;
	}

	const SDL_VideoInfo *video = SDL_GetVideoInfo();
	fullscreenw = video->current_w; fullscreenh = video->current_h;
	ERR("Screen resolution is: %d,%d\n", fullscreenw, fullscreenh);

	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

	sdl_surface_init(optWindow);
	
	SDL_AudioSpec *desired;
	desired=(SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
	desired->freq = 44100;
	desired->format = AUDIO_S16SYS;
	desired->channels = 1; // stereo is a stupid gimmick anyway
	desired->samples = 4096; // FIXME
	desired->callback = audio_callback;
	desired->userdata = NULL;
	int failure = SDL_OpenAudio(desired, NULL);
	if ( failure < 0 ){
	  REALERR("Couldn't open audio: %s\n", SDL_GetError());
	  Quit(1);
	}
	free(desired);
	
#if LOG_AUDIO
	audiolog = fopen("/tmp/OUTAUDIO", "w");
#endif
	
	SDL_PauseAudio(0);
	
	SDL_EnableUNICODE(true);
	
	ERR("Found %d joysticks\n", SDL_NumJoysticks());
	if (SDL_NumJoysticks() > 0) {
		SDL_JoystickEventState(SDL_ENABLE);
		SDL_JoystickOpen(0);
		gotJoystick = true;
	}
			
	{ // Load high scores
		bool failure = false;
		char optfile[FILENAMESIZE] = "jumpman.sav";
		
		ifstream f;
		f.open(optfile, ios_base::in | ios_base::binary);
		if (!f.fail()) {
			f.seekg (0, ios::end);
ERR("ZZZZ File size %d\n", (int)f.tellg());
			if (sizeof(int) <= f.tellg()) {
				f.seekg (0, ios::beg);
				unsigned int count;
				f.read((char *)&count, sizeof(count)); count = ntohl(count);
ERR("ZZZZ count %d\n", count);
				for(int c = 0; c < count; c++) {
					unsigned int namelen, listlen;
					char filename[FILENAMESIZE+1];
					
					f.read((char *)&namelen, sizeof(namelen)); namelen = ntohl(namelen);
					if (namelen > FILENAMESIZE)
						break;
					f.read(filename, namelen);
					filename[namelen] = '\0';
					
ERR("ZZZZ File: %s\n", filename);
	  
					pair<scoreinfo,scoreinfo> &s = scores[filename];
					
					f.read((char *)&listlen, sizeof(listlen)); listlen = ntohl(listlen);
ERR("ZZZZ list %d\n", listlen);					
					for(int d = 0; d < listlen; d++) { 	// Size will be enforced by push_back.
						unsigned int temp;
						
						f.read((char *)&temp, sizeof(temp)); temp = ntohl(temp);
						s.first.time.push_back(temp);
ERR(" ZZZZ a: %d", temp);
						
						f.read((char *)&temp, sizeof(temp)); temp = ntohl(temp);
						s.first.deaths.push_back(temp);
ERR(" ZZZZ b: %d", temp);
						
						f.read((char *)&temp, sizeof(temp)); temp = ntohl(temp);
						s.second.time.push_back(temp);
ERR(" ZZZZ c: %d", temp);

						f.read((char *)&temp, sizeof(temp)); temp = ntohl(temp);
						s.second.deaths.push_back(temp);
ERR(" ZZZZ d: %d\n", temp);
					}
				}
			} else failure = true;
		} else failure = true;
		if (failure) {		
			// We have no high scores. Is this a problem?
		}
	}
	
	haveWonGame = (scores["/"].first.time.size() == 10 && scores["/"].first.time[9]); // I.E. if you've won the game, you have 10 high scores for Main.jmp
	ERR("Main.jmp has: %d paths %s\n", scores["/"].first.time.size(), haveWonGame?"(Winner)":"");
}

void plateinfo::reconstruct() {
	this->displays[0] = glGenLists(frames);
	
	for(int c = 1; c < frames; c++)
		this->displays[c] = this->displays[0] + c;
	
	for(int c = 0; c < frames; c++) {
		slice *thing = this->slices[c]->clone();

		thing->construct();

		double scale = 2.4;
		glNewList(this->displays[c],GL_COMPILE);
		glRotatef(180,0,0,1);
		glTranslatef(-(thing->width*scale)/2,-(thing->height*scale)/2,0);
		for(vector<block>::iterator b = thing->blocks.begin(); b != thing->blocks.end(); b++) {
			glBegin(GL_QUADS); // Duplication?
					
//			for(int i=0; i<4; i++){
				   glVertex2f(b->x*scale, b->y*scale);
				   glVertex2f(b->x*scale, (b->y + b->height)*scale);
				   glVertex2f((b->x + b->width)*scale, (b->y + b->height)*scale);
				   glVertex2f((b->x + b->width)*scale, b->y*scale);
//			}
			glEnd();
		}
		glEndList();
		
		delete thing;
	}
}

plateinfo *plate_construct(const char *fmt, int count, float r, float g, float b, platebehave behave)
{
	plateinfo *s = new plateinfo();
	s->displays = new unsigned int[count];
	s->frames = count;
	s->r = r; s->g = g; s->b = b; s->behave = behave;
	s->slices = new slice *[count];
	
	for(int c = 0; c < s->frames; c++) {
		char filename[FILENAMESIZE];
		
		internalPath(filename, fmt, c+1);

		s->slices[c] = new slice();
		s->slices[c]->construct(filename, false); // Doesn't REALLY construct
		
		s->width = s->slices[c]->width;
		s->height = s->slices[c]->height; // If this isn't the same on each pass, you're doing it wrong
	}
	
	s->reconstruct();
				
	return s;
}

void drawCircle(cpFloat x, cpFloat y, cpFloat r, cpFloat a)
{
	const int segs = 15;
	const cpFloat coef = 2.0*M_PI/(cpFloat)segs;
	
	int n;
		
	glBegin(GL_LINE_LOOP); {
		for(n = 0; n < segs; n++){
			cpFloat rads = n*coef;
			glVertex2f(r*cos(rads + a) + x, r*sin(rads + a) + y);
		}
		glVertex2f(x,y);
	} glEnd();
}

void drawCircleShape(cpShape *shape)
{
	cpBody *body = shape->body;
	cpCircleShape *circle = (cpCircleShape *)shape;
	cpVect c = cpvadd(body->p, cpvrotate(circle->c, body->rot));
	drawCircle(c.x, c.y, circle->r, body->a);
}

void drawSegmentShape(cpShape *shape)
{
	cpBody *body = shape->body;
	cpSegmentShape *seg = (cpSegmentShape *)shape;
	cpVect a = cpvadd(body->p, cpvrotate(seg->a, body->rot));
	cpVect b = cpvadd(body->p, cpvrotate(seg->b, body->rot));
	
	glBegin(GL_LINES); 
		  Color3f(1.0, 1.0, 1.0);{
		glVertex2f(a.x, a.y);
		glVertex2f(b.x, b.y);
	} glEnd();
}

void drawSplosions(spaceinfo *s) { // Someday will other things splode?
	//glDisable(GL_DEPTH_TEST);
	while (!s->splodes.empty() && s->splodes.back().ct >= s->splodes.back().tt) {
		s->splodes.pop_back();			
	}
	for(list<splode_info>::iterator b = s->splodes.begin(); b != s->splodes.end(); b++) {
		splode_info &i = *b;
		
		double zoom = (double)i.ct/i.tt;
		glPushMatrix();
		glTranslatef(i.p.x,i.p.y,0.0);
		glTranslatef(0,0,0);
		glRotatef((chassis->a + M_PI)/M_PI*180,0,0,1);
		if (i.reflect)
			glRotatef(180,0,1,0);
		glColor4f(i.r,i.g,i.b, (1-zoom)*(1-zoom));      // Color Orange?
		for(int x = 0; x < i.sploding->width; x++) {
			for(int y = 0; y < i.sploding->height; y++) {
				if (i.sploding->pixel[x][y]) {
					double scale = 2.4;
							glBegin(GL_QUADS); // Duplication?
					double czoom = 1+100*zoom;
					double pzoom = 1 + 5*zoom;
					double x_ = x - i.sploding->width/2;
					double y_ = y - i.sploding->height/2;
					x_ *= czoom; y_ *= czoom;
				   glVertex2f(x_*scale, y_*scale);
				   glVertex2f(x_*scale, (y_ + pzoom)*scale);
				   glVertex2f((x_ + pzoom)*scale, (y_ + pzoom)*scale);
				   glVertex2f((x_ + pzoom)*scale, y_*scale);
					glEnd();

				}
			}
		}
		glPopMatrix();
		
		i.ct++;
		i.adjust_sound();
	}
	//glEnable(GL_DEPTH_TEST);
}

void drawTrails(spaceinfo *s) { // Someday will other things splode?
//	glDisable(GL_DEPTH_TEST);
	glPushMatrix();
	glTranslatef(0,0,0);
	while (!s->trails.empty() && s->trails.back().ct >= s->trails.back().tt) {
		s->trails.pop_back();			
	}
	glBegin(GL_LINES);
	for(list<trail_info>::iterator b = s->trails.begin(); b != s->trails.end(); b++) {
		trail_info &i = *b;
		
		double alpha = 1 - (double)i.ct/i.tt;
		glColor4f(i.r,i.g,i.b, alpha);      // Color Orange?
		glVertex2f(i.at.x, i.at.y);
		glVertex2f(i.at.x+i.trail.x*36, i.at.y+i.trail.y*36);
		i.ct++;
	}
	glEnd();
	glPopMatrix();
//	glEnable(GL_DEPTH_TEST);
}

int currentJumpmanFrame() {
	int whichframe = 2;
	if (jumping) {
		whichframe = 1;
	} else if (input_power) {
		int frames[5] = {0, 0, 3, 3, 2};
		unsigned int frame = (ticks - started_moving_at_ticks);
		frame /= 10;
		frame %= 5;
		whichframe = frames[frame];
	}
	return whichframe;
}

bool restrictDraw = false, restrictDrawObject = false;
unsigned int restrictDrawTo = 0, restrictDrawToInt = 0;
bool drawingNoRots = false;

void drawPolyShape(cpShape *shape, void *_data)
{
	if (!((shape->collision_type == C_NOROT) ^ !(drawingNoRots))) return; // Continue only if both false or both true

	cpBody *body = shape->body;
	spaceinfo *data = (spaceinfo *)_data;
	plateinfo *slice = pinfo[shape->collision_type];
	cpVect p = body->p;
	
	bool forceL = data->layers > 0;
	if (forceL && restrictDraw && (!(restrictDrawTo & shape->layers) || !C_FLOORTYPE(shape->collision_type)))
		return;

	if (restrictDrawObject && C_FLOORTYPE(shape->collision_type))
		return;

	if (data->panProblem && exiting_entire_game && pantype == pan_deep && !C_FLOORTYPE(shape->collision_type))
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
				double dx = cpvdot( cpvsub(chassis->p, shape->body->p), shape->body->rot );
				if (data->repeat) {
					double altdx = dx + data->repeat_every * (dx>0 ? -1:1);
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

	case C_NOROT:
	{
		cpPolyShape *poly = (cpPolyShape *)shape;
		int num = poly->numVerts;
		cpVect *verts = poly->verts;
		
		glBegin(GL_QUADS);
		for(int i=0; i<num; i++){
			   cpVect v = cpvadd(body->p, cpvrotate(verts[i], body->rot));
			   glVertex3f(v.x, v.y, 0);
	   }
		glEnd();
	} break;

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
				
			glPushMatrix(); // And this! This is just ugly!
			glTranslatef(p.x,p.y,0);
			glRotatef(body->a/M_PI*180,0,0,1);
			glCallList(slice->displays[whichframe]);
			glPopMatrix();

			have_slice = false;
		}
	} break;
	
	case C_REENTRY: {
		if (edit_mode == EWalls) {
			cpPolyShape *poly = (cpPolyShape *)shape;
			int num = poly->numVerts;
			cpVect *verts = poly->verts;

			glLineWidth(4);
			if (!forceL)
				Color3f(0.5,0.5,0.5);
			else
				BlindColor3f(data->r[l],data->g[l],data->b[l], shape->layers);
					glBegin(GL_LINE_LOOP);
		   for(int i=0; i<num; i++){
				   cpVect v = cpvadd(body->p, cpvrotate(verts[i], body->rot));
				   cpVect v2 = verts[(i+2)%num];
				   v.x += (v2.x > v.x ? 2.4 : -2.4);
				   v.y += (v2.y > v.y ? 2.4 : -4.4); 
				   
				   glVertex3f(v.x, v.y, 0);
		   } 
		   glEnd();

			glLineWidth(1);
		}
	} break;

	case C_FLOOR: case C_LAVA: case C_LOOSE: case C_LOOSE2: {
		cpPolyShape *poly = (cpPolyShape *)shape;
		int num = poly->numVerts;
		cpVect *verts = poly->verts;
		
		glBegin(GL_QUADS);
		if (shape->collision_type == C_LAVA)
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
	   for(int i=0; i<num; i++){
			   cpVect v = cpvadd(body->p, cpvrotate(verts[i], body->rot));
			   glVertex3f(v.x, v.y, 0);
	   }
		glEnd();
		} break;
	default: {
/*
		glPushMatrix();
		glBegin(GL_QUADS);
		  Color3f(1.0,1.0,1.0);
	   for(int i=0; i<num; i++){
			   cpVect v = cpvadd(body->p, cpvrotate(verts[i], body->rot));
			   glVertex3f(v.x, v.y, 0);
	   }
		glEnd();
		glPopMatrix();
*/
		break;}
	}
	
	if (have_slice) {
			if (body->data) { // Seems inefficient?
				enemy_info *info = (enemy_info *)body->data;
				info->lastWhichFrame = whichframe;
				info->lastReflect = reflect;
			}
	
			glPushMatrix();
			glTranslatef(p.x,p.y,0);
			glRotatef(body->a/M_PI*180,0,0,1);
			if (reflect)
				glRotatef(180,0,1,0);
			if (!forceL)
			  Color3f(slice->r,slice->g,slice->b);      // Color Orange
			else
			  BlindColor3f(data->r[l],data->g[l],data->b[l], shape->collision_type != C_PAINT ? shape->layers : ~shape->layers);
			glCallList(slice->displays[whichframe]);
			glPopMatrix();
	}
	
#if DRAW_DEBUG
	if (!C_BALLTYPE(shape->collision_type)) {
		cpPolyShape *poly = (cpPolyShape *)shape;
		int num = poly->numVerts;
		cpVect *verts = poly->verts;
		int i;
		glBegin(GL_LINE_LOOP);
		glColor3f(0.5f,0.5f,0.5f);
	   for(i=0; i<num; i++){
			   cpVect v = cpvadd(body->p, cpvrotate(verts[i], body->rot));
			   glVertex3f(v.x, v.y, 0);
	   } glEnd();
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

void drawObject(void *ptr, void *data)
{
	cpShape *shape = (cpShape*)ptr;
#if 1
	drawPolyShape(shape, data);
#else
	switch(shape->klass->type){
		case CP_CIRCLE_SHAPE:
			drawCircleShape(shape);
			break;
		case CP_SEGMENT_SHAPE:
			drawSegmentShape(shape);
			break;
		case CP_POLY_SHAPE:
			drawPolyShape(shape, data);
			break;
		default:
			ERR("Bad enumeration in drawObject().\n");
	}
#endif
}

void draw_debug_Object(void *ptr, void *data) // FIXME: Too much copypaste
{
	cpShape *shape = (cpShape*)ptr;
	cpBody *body = shape->body;
	if (body) {
		glBegin(GL_POINTS); {
			glVertex2f(body->p.x, body->p.y);
		} glEnd();

#if AXIS_DEBUG
		glBegin(GL_LINES); { 
			glColor3f(1.0, 0.0, 1.0);
			glVertex2f(body->p.x, body->p.y);
			glVertex2f(body->p.x+body->rot.x*36, body->p.y+body->rot.y*36);
			if (body->data) { // Seems inefficient?
				enemy_info *info = (enemy_info *)body->data;
				glColor3f(0.0, 1.0, 1.0);
				glVertex2f(body->p.x, body->p.y);
				glVertex2f(body->p.x+info->tiltg.x*36, body->p.y+info->tiltg.y*36);
			}
		} glEnd();
#endif
	}
}

inline int imax(int a, int b) { return a > b ? a : b; }

void drawButton(void *ptr, void *data)
{
	cpPolyShape *poly = (cpPolyShape *)ptr;
	ControlBase *control = (ControlBase *)poly->shape.data;
	int num = poly->numVerts;
	cpVect *verts = poly->verts;
	
	if (!control) return;

	// Note we do no body-specific translation because we assume this is the UI layer.

	if (control->bg) {
		glBegin(GL_QUADS);
		if (control == KeyboardControl::focus || control->highlighted)
			glColor4f(0.5,0.5,1.0,0.5);	
		else if (!haveDoneBlue)
			glColor4f(0.25,0.25,0.25,0.75);
		else
			glColor4f(0.75,0.75,0.75,0.75); 
		for(int i=0; i<num; i++){
			   cpVect v = verts[i];
			   glVertex3f(v.x, v.y, 0);
		}
		glEnd();
	}

	bool floatOff = !control->text.empty() && control->img;
	const double border = 2.4*2;
	const double slicesize = 72 - border*2; // I have no idea

	if (!control->text.empty()) {
		int count = 1, index = 0;
		string::size_type upto = 0, from = 0;
		while (string::npos != (upto = control->text.find('\n', upto+1))) // Assumes the first character is never a \n.
			count++;
		
		upto = 0;
		from = 0;
		do {
			upto = control->text.find('\n', upto+1);
			string sub = control->text.substr(from, string::npos == upto ? control->text.size() : upto-from);
		
			glPushMatrix();
			if (!haveDoneBlue)
				glColor3f(1.0,1.0,1.0);	
			else
				glColor3f(0.0,0.0,0.0);	
			glTranslatef(control->p.x, control->p.y, 0);
			glTranslatef(-centerOff(sub.c_str()), (count/2.0-index-1)*floatHeight, 0);
			if (floatOff) 
				glTranslatef(slicesize/2, 0, 0);
			glEnable( GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, ftex);
			floating->Render(sub.c_str());
			glDisable( GL_TEXTURE_2D); 
			glPopMatrix();
			
			index++;
			from = upto + 1;
		} while (upto != string::npos);
	}
	
	if (control->img) {
		const double scale = slicesize/imax(control->img->width, control->img->height), 
			pzoom = fmax(scale, fmax(oneOrthX, oneOrthY)); // SO NOT SURE ABOUT THIS
//		ERR("Orth %lf,%lf\n", oneOrthX, oneOrthY);
		glPushMatrix();
		glTranslatef(control->p.x, control->p.y, 0);
		glTranslatef(-slicesize/2, slicesize/2, 0);
		if (floatOff)
			glTranslatef(-90 + slicesize/2 + border, 0, 0);
		for(int x = 0; x < control->img->width; x++) {
			for(int y = 0; y < control->img->height; y++) {
				uint32_t pixel = control->img->pixel[x][y];
				unsigned char *rgb = (unsigned char *)&pixel;
				
				if (!pixel) continue;
				
//				ERR("Color %d,%d,%d,%d %s%s\n", (int)rgb[0], (int)rgb[1], (int)rgb[2], (int)rgb[3], pixel == C_JUMPMAN ? "JUMPMAN":"", pixel == C_MARCH ? "MARCH":"");
				
				glBegin(GL_QUADS);					
				if (pixel == C_FLOOR)
					glColor3f(1.0,1.0,1.0);
				else {
					pixel = ntohl(pixel);
					glColor3f(rgb[0]/255.0, rgb[1]/255.0, rgb[2]/255.0);
				}
				glVertex2f(x*scale, -y*scale);
				glVertex2f(x*scale, -y*scale - pzoom);
				glVertex2f(x*scale + pzoom, -y*scale - pzoom);
				glVertex2f(x*scale + pzoom, -y*scale);
				glEnd();
			}
		}
		glPopMatrix();
	}
}

void drawCollisions(void *ptr, void *data)
{
	cpArbiter *arb = (cpArbiter*)ptr;
	int i;
	for(i=0; i<arb->numContacts; i++){
		cpVect v = arb->contacts[i].p;
		glVertex2f(v.x, v.y);
	}
}

// Code duplication :(
void drawCollisionNormals(void *ptr, void *data)
{
	cpArbiter *arb = (cpArbiter*)ptr;
	int i;
	for(i=0; i<arb->numContacts; i++){
		cpVect v = arb->contacts[i].p;
		glVertex2f(v.x, v.y);
		glVertex2f(v.x + arb->contacts[i].n.x*36, v.y + arb->contacts[i].n.y*36);
	}
}

char *controlNames[] = {"Left", "Right", "Jump", "Turn Counter-Clockwise", "Turn Clockwise", "Edit Controls", "Quit"};
char *readableControl(int c);
void controlsDraw(bool startdone, bool done, bool moveOn, int currentControl, int currentError) {
	Color3f(1.0,1.0,1.0);
	
	glPushMatrix();
	glTranslatef(-centerOff("Controls"), 200, 0);
	floating->Render("Controls");
	glPopMatrix();
	
	float leftSide = -centerOff("(Press a key or gamepad button to set the highlighted control)");
	float rightSide = -leftSide - 2*centerOff("[Right arrow]"); // Should be the longest thing we know how to render?
	
	glPushMatrix();
	glTranslatef(leftSide, 200 - floatHeight*2, 0);
	if (!startdone)
		floating->Render("(Press a key or gamepad button to set the highlighted control)");
	glPopMatrix();
	
	for(int c = 0; c < NUMCONTROLS; c++) {
		int line = c;
		if (line >= NUMEDITCONTROLS) line++;
		
		glPushMatrix();
		if (c == currentControl && !done) {
			if (moveOn)
				Color3f(0.0,1.0,1.0);
			else
				Color3f(1.0,1.0,0.0);
		} else if (c == currentError)
			Color3f(1.0,0,0);
		else
			Color3f(1.0,1.0,1.0);
			
		glTranslatef(leftSide,  - floatHeight*line, 0);
		floating->Render(controlNames[c]);
		glPopMatrix();
		
		glPushMatrix();
		glTranslatef(rightSide, - floatHeight*line, 0);
		floating->Render(readableControl(c));
		glPopMatrix();
	}
	
	if (currentError >= 0) {
		Color3f(1.0,1.0,1.0);
		glPushMatrix();
		glTranslatef(-centerOff("That button is already in use."), -floatHeight*(NUMCONTROLS+2), 0);
		floating->Render("That button is already in use.");
		glPopMatrix();
	}
	
	if (done) {
		Color3f(1.0,1.0,1.0);
		glPushMatrix();
		glTranslatef(-centerOff("Press any key to continue."), -floatHeight*(NUMCONTROLS+2), 0);
		floating->Render("Press any key to continue.");
		glPopMatrix();
	}
}

double lastpanfadez;

inline double panfade(double _from, double _to) {
	double zoom = cos(M_PI/2 * (double)panct/pantt);
	double azoom = 1-zoom;
	double scale = -1;//36;
	double from = scale*_from;
	double to = scale*_to;
	return from*zoom+to*azoom;
}

void displayStaticFor(spaceinfo *s) { // Put in its own function so I can do a funny trick with stencils before drawing.
	bool mustNorot = s->has_norots && (s->num == jumpman_s || (pantype == pan_deep && s->num == jumpman_s + exit_direction));
	if (mustNorot) {
		glEnable(GL_STENCIL_TEST);						// Enable Stencil Buffer For "marking" The Floor
		glClear(GL_STENCIL_BUFFER_BIT);
		
		glStencilFunc(GL_NEVER, 1, ~0);						// Never passes (everything invisible), reference 1
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);				// We Set The Stencil Buffer To 1 Where We Draw Any Polygon
		
		drawingNoRots = true;
		cpSpaceHashEach(s->space->staticShapes, &drawObject, s);
		drawingNoRots = false;
		
		// We have now built up a stencil buffer with the pixels to be drawn into and are ready to draw our content:
		int l = restrictDraw ? restrictDrawToInt : 0;  // This feature requires so much state to be tracked and none of it is used for anything else...
		double r = s->r[l], g = s->g[l], b = s->b[l], a = 0.25;
		
		if (pantype == pan_deep && ((s->num == jumpman_s) ^ (exit_direction > 0))) {
			a = -panfade(0,1);
			if (s->num == jumpman_s)
				a = 1 - a;
			a *= 0.25;
		}
		
		int fps34 = FPS*3.0/4;
		if (ticks - (fps34) < rotYellingSince && (!restrictDraw || restrictDrawTo & chassisShape->layers)) {
			double fade = ticks - rotYellingSince; fade /= (fps34);
			double unfade = (1-fade);
			double h,s,v, r2, g2, b2;
						
			RGBtoHSV( r, g, b, &h, &s, &v );
			s = 1.0;
			if (isnan(h)) h = 0;
			else h = fmod(h + 180, 360);
			if (v < 0.1) v = 0.1;
			HSVtoRGB( &r2, &g2, &b2, h, s, v );

			r = r*fade+r2*unfade;
			g = g*fade+g2*unfade;
			b = b*fade+b2*unfade;
			a *= (2-fade);
		}
		
		glStencilFunc(GL_EQUAL, 1, ~0);						// Passes when equal, reference 1
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);				// We don't care what happens to the stencil buffer anymore
	
		if (cam_fixed == s->camera) {
			glPushMatrix();
			glRotatef(s->staticBody->a/M_PI*180,0,0,1);
		}
		
		glBegin(GL_QUADS);
		glColor4f(r, g, b, a);
		glVertex3f(-s->base_width/2, -s->base_width/2, 0);
		glVertex3f(-s->base_width/2, s->base_width/2, 0);
		glVertex3f(s->base_width/2, s->base_width/2, 0);
		glVertex3f(s->base_width/2, -s->base_width/2, 0);
		glEnd();
		
		if (cam_fixed == s->camera)
			glPopMatrix();
		
		glDisable(GL_STENCIL_TEST);						// Enable Stencil Buffer For "marking" The Floor
	}
	
	cpSpaceHashEach(s->space->staticShapes, &drawObject, s);
}

void display(void)
{	
	if (!optSplatter)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else
		glClear(GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	
	glTranslatef(-scan_x, scan_y, 0);

	if (surplusSpin)
		glRotatef(surplusSpin/M_PI*180.0, 0,0,1);

	if (!(cam_fixed == level[jumpman_s].camera)) {
		if (pantype != pan_deep) {
			glRotatef(-roten/M_PI * 180.0,0,0,1);
		} else {
			double nroten = frot(0); // round to something reasonable
			glRotatef(panfade(nroten, surplusSpin)/M_PI * 180.0,0,0,1);
		}
	}
	
	glTranslatef(rescan.x, rescan.y, 0);

	if (pantype != pan_dont) {	
        lastpanfadez = panfade(panfz,pantz); // Helps us know what to draw
//		ERR("deep! %lf\n", panfade(panfz,pantz));
		glTranslatef(panfade(panf.x,pant.x), panfade(panf.y,pant.y), lastpanfadez);
	} else if (cam_fixed == level[jumpman_s].camera || edit_mode == EWalls) {
		glTranslatef(0, 0, -jumpman_d);
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
//		ERR("deep %d\n", -jumpman_d);
		glTranslatef(-p.x, -p.y, -jumpman_d);
	}
		
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
	
	  Color3f(0.0, 0.0, 0.0);

  for(int c = level.size()-1; c >= 0; c--) { // FIXME: Forward or backward?
  	spaceinfo *s = &(level[c]);
	if (!okayToDraw(s->deep))
		continue;
		
	bool repeat = s->repeat && !(drawGrid && edit_mode == EWalls);
	bool sploded = false;
		
	if (repeat) {
		glNewList(tempdl[0],GL_COMPILE);
	}
	
	glDisable(GL_DEPTH_TEST);
	glPushMatrix();
		
	if (!repeat)
		glTranslatef(0, 0, s->deep);	
		
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
			
#if DRAW_DEBUG
	cpArray *bodies = s->space->bodies;
	
	glBegin(GL_POINTS); {
		glColor3f(0.0, 0.0, 1.0);
		for(int i=0; i<bodies->num; i++){
			cpBody *body = (cpBody*)bodies->arr[i];
			glVertex2f(body->p.x, body->p.y);
		}
		
		glColor3f(1.0, 0.0, 0.0);
		cpArrayEach(s->space->arbiters, &drawCollisions, NULL);
		
		glColor3f(0.0, 1.0, 0.0);
		glVertex2f(jumpman_x, jumpman_y);
	} glEnd();
	
#if AXIS_DEBUG
	glBegin(GL_LINES); { 
		for(int i=0; i<bodies->num; i++){
			cpBody *body = (cpBody*)bodies->arr[i];
			glColor3f(1.0, 0.0, 1.0);
			glVertex2f(body->p.x, body->p.y);
			glVertex2f(body->p.x+body->rot.x*36, body->p.y+body->rot.y*36);
			if (body->data) { // Seems inefficient?
				enemy_info *info = (enemy_info *)body->data;
				glColor3f(0.0, 1.0, 1.0);
				glVertex2f(body->p.x, body->p.y);
				glVertex2f(body->p.x+info->tiltg.x*36, body->p.y+info->tiltg.y*36);
			}
		}
		
		glColor3f(1.0, 0.0, 0.0);
		cpArrayEach(s->space->arbiters, &drawCollisionNormals, NULL);
	} glEnd();
#endif

	for(hash_map<unsigned int, cpShape *>::iterator b = s->mans.begin(); b != s->mans.end(); b++)
		draw_debug_Object((*b).second, s);
	for(hash_map<unsigned int, cpShape *>::iterator b = s->tilt.begin(); b != s->tilt.end(); b++)
		draw_debug_Object((*b).second, s);

#endif


		if (repeat) {
			glPopMatrix();
			glEndList();
			glNewList(tempdl[1],GL_COMPILE);
		}

		if (!s->splodes.empty()) {
			drawSplosions(s);
			sploded = true;
		}
			
#if DRAW_DEBUG
		if (!s->trails.empty()) {
			drawTrails(s);
			sploded = true;
		}
#endif
		if (!repeat)
			glPopMatrix();
		
		if (repeat) {
			glEndList();
			GLdouble model_view[16];
			GLdouble projection[16];
			GLint viewport[4];	
			GLdouble buffZ, tempX, tempY, tempZ;
			
			if (cam_fixed == s->camera) {
				glPushMatrix();
				glRotatef(s->staticBody->a/M_PI*180,0,0,1);
			}
			glGetDoublev(GL_MODELVIEW_MATRIX, model_view);
			glGetDoublev(GL_PROJECTION_MATRIX, projection);
			glGetIntegerv(GL_VIEWPORT, viewport);
			if (cam_fixed == s->camera)
				glPopMatrix();
			
			GLdouble x[4], y[4];
						
			gluProject(0,0,s->deep, model_view, projection, viewport, &tempX, &tempY, &buffZ);
			gluUnProject(0,0,buffZ, model_view, projection, viewport, &x[0], &y[0], &tempZ);
			gluUnProject(0,surfaceh,buffZ, model_view, projection, viewport, &x[1], &y[1], &tempZ);
			gluUnProject(surfacew,0,buffZ, model_view, projection, viewport, &x[2], &y[2], &tempZ);
			gluUnProject(surfacew,surfaceh,buffZ, model_view, projection, viewport, &x[3], &y[3], &tempZ);

#if 0
			GLdouble &l = x[0], &u = y[0];
			GLdouble &r = x[3], &d = y[3];
			ERR("s %d: u %lf d %lf l %lf r %lf t %lf\n", s->deep, u, d, l, r, tempZ);
//			glDisable(GL_DEPTH_TEST);
			glBegin(GL_QUADS); // Duplication?
			glColor3f(1.0, 0.0, 1.0);
			glVertex3f(l-36, u-36, tempZ); // Actually displays in lower left
			glVertex3f(l+36, u-36, tempZ);
			glVertex3f(l+36, u+36, tempZ);
			glVertex3f(l-36, u+36, tempZ);
			glColor3f(1.0, 1.0, 0.0);
			glVertex3f(r-36, d-36, tempZ); // Actually displays in upper right
			glVertex3f(r+36, d-36, tempZ);
			glVertex3f(r+36, d+36, tempZ);
			glVertex3f(r-36, d+36, tempZ);
			glEnd();		
//			glEnable(GL_DEPTH_TEST);
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
				
//				ERR("WILLDRAW %d: [%lf,%lf]->[%lf,%lf] == [%d,%d]->[%d,%d]\n", s->num, x[0]/s->repeat_every, y[0]/s->repeat_every, x[3]/s->repeat_every, y[3]/s->repeat_every, x1, y1, x2, y2);
			}
		
			for(int l = 0; l < 2; l++) {
				if (0 == l || sploded) {
					for(int y = y1; y <= y2; y++) {
						for(int x = x1; x <= x2; x++) {
							glPushMatrix();
							//int rotby = y%2?3-((x + (y/2)*2)%4):(x + (y/2)*2)%4;
							
							int rr = s->tiltfor(x,y);
							int rotby = rr&REPROTROT;
							int reflect = rr&REPROTREF;

							cpVect offset = {x*s->repeat_every, y*s->repeat_every};

							if (s->num == jumpman_s && cam_fixed == s->camera) {
								cpVect fixedRotateBy = s->staticBody->rot;
								if (reflect)
									fixedRotateBy = cpvneg(fixedRotateBy);
								offset = cpvrotate(offset, fixedRotateBy);						
							}
							
							glTranslatef(offset.x, offset.y, 0);
							
							glTranslatef(0, 0, s->deep);	

							if (rotby)
								glRotatef(90*rotby, 0,0,1);
								
							if (reflect) {
								if (s->num == jumpman_s && cam_fixed == s->camera) // SO I SERIOUSLY HAVE NO IDEA WHY THIS NEXT LINE WORKS AND IT CONCERNS ME GREATLY
									glRotatef(2*cpvtoangle(s->staticBody->rot)/(M_PI)*180, 0,0,1);
								glRotatef(180,0,1,0);
							}

							glCallList(tempdl[l]);
							glPopMatrix();
						}
					}
				}
			}
		}
			
		glEnable(GL_DEPTH_TEST);
	}
	
if (!doingControlsScreen) { // Following are a bunch of "floating" graphics, none of which should appear in the controls screen
	if (edit_mode == EWalls && drawGrid) {
		glDisable(GL_DEPTH_TEST);
		glPushMatrix();
		glColor4f(72.0/255, 99.0/255, 128.0/255, 0.5);
//		GLdouble lnePerX = onePerX * scan_r, // FIXME: Blatant code duplication 
//		 lnePerY = onePerY * scan_r;
		//GLdouble lriginPerX = originPerX + scan_x*lnePerX,
		// lriginPerY = originPerY + scan_y*lnePerY;
		 
		for(int c = editLayer*2; c < editLayer*2+2 && c < editSlice.size(); c++) {
		 int px = editSlice[c]->width/2, py = editSlice[c]->height/2;
		 int ox = editSlice[c]->width%2, oy = editSlice[c]->height%2;
		 if (c%2) glRotatef(45,0,0,1);
		 
		 double xoff = ox ? 18 : 0, yoff = oy ? 18 : 0; 
		 double xfrom = -36*px-xoff, xto = 36*px+xoff;
 		 double yfrom = -36*py-yoff, yto = 36*py+yoff;
//		ERR("STARTDRAW jumpman at %lf %lf", (double)chassis->p.x, (double)chassis->p.y); 
			glBegin(GL_LINES); 
			 for(double c = xfrom; c <= xto; c+=36) {
//			ERR("DRAWX line %lf, %lf -> %lf, %lf", (double)c, (double)yfrom, (double)c, (double)yto);
				glVertex3f(c, yfrom, jumpman_d-1);
				glVertex3f(c, yto, jumpman_d-1);
			}
			 for(double c = yfrom; c <= yto; c+=36) {
//			ERR("DRAWY line %lf, %lf -> %lf, %lf", (double)xfrom, (double)c, (double)xto, (double)c);
				glVertex3f(xfrom, c, jumpman_d-1);
				glVertex3f(xto, c, jumpman_d-1);
			}
			glEnd();
		}
		
		glBegin(GL_POINTS); {
			glColor3f(0.0, 1.0, 0.0);
			glVertex3f(0, 0, jumpman_d-1);
		} glEnd();
		
		glPopMatrix();
		glEnable(GL_DEPTH_TEST);
	}

	if (doingEnding) {
		glDisable(GL_DEPTH_TEST);
		glPushMatrix();

#define BPM 120
#define SPAN ((44100*60)/BPM)
		int mountainat = (endingAt/(SPAN)*2 - 256 - 16);
		if (mountainat > 14) mountainat = 14;
		glTranslatef(0, mountainat*4.8, -1);
							
		glCallList(mountainl);
			
		glPopMatrix();
				
		if (endingAtMode == 9 && ((ticks / 30) % 2)) {
			const char * pressakey = "PRESS A KEY TO CONTINUE";
			glPushMatrix();
			
			goOrtho();

			glColor3f(0.0,0.0,0.0);	
			glEnable( GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, ftex);

			glTranslatef(-centerOff(pressakey) + 750.0/2 * (2.0/3), -floatHeight*(1/2.0), 0);
			floating->Render(pressakey);
			
			glDisable( GL_TEXTURE_2D); 

			glPopMatrix();
		}
		
		glEnable(GL_DEPTH_TEST);

		
		endingTickMainThread();
	}

	if (edit_mode == EWalls && haveLineStart) {
		glDisable(GL_DEPTH_TEST);
		glPushMatrix();
		glColor4f(1-72.0/255, 1-99.0/255, 1-128.0/255, 0.5);

		double fcurrent = roten / (M_PI/4);
		int current = (fabs(fcurrent) + 0.5); // WTF				
		current %= 2;
		current += (editLayer * 2);
		if (!(current >= editSlice.size())) {
			 int px = editSlice[current]->width/2, py = -editSlice[current]->height/2;
			 int ox = editSlice[current]->width%2, oy = editSlice[current]->height%2;
			 px -= lineStartX; py += lineStartY;
			 if (current%2) glRotatef(45,0,0,1);
			 
			 double xoff = ox ? 18 : 0, yoff = oy ? 18 : 36; // Without the -36, X is one square off. Why? 
			 double xfrom = -36*px-xoff;
			 double yfrom = -36*py-yoff;

			glBegin(GL_LINES); 
			glVertex3f(xfrom, yfrom, jumpman_d-1);
			glVertex3f(xfrom+36, yfrom+36, jumpman_d-1);
			glVertex3f(xfrom, yfrom+36, jumpman_d-1);
			glVertex3f(xfrom+36, yfrom, jumpman_d-1);
			glEnd();
		}
		
		glPopMatrix();
		glEnable(GL_DEPTH_TEST);
	}

#if ZERO_DEBUG
	glDisable(GL_DEPTH_TEST);
	glPushMatrix();
	glTranslatef(0, 0, jumpman_d);
	glBegin(GL_LINES); { 
		glColor3f(1.0, 0.0, 1.0);
		cpVect v = cpvrotate(cpv(1,0), cpvforangle(roten));
		glVertex2f(0, 0);
		glVertex2f(v.x*36, v.y*36);
		glVertex2f(0, 0);
		glVertex2f(chassis->p.x, chassis->p.y);
	} glEnd();
	glBegin(GL_POINTS); {
		glColor3f(0.0, 1.0, 0.0);
		glVertex2f(0, 0);
	} glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
#endif

	if (EAngle == edit_mode) { // Angle hints 1
		glLoadIdentity();
		goOrtho();
		glDisable(GL_DEPTH_TEST);
	
		// DRAW HERE
		for(int c = 0; c < 8; c++) {
			glPushMatrix();
			glRotatef(180 + 45*c,0,0,1);
			glTranslatef(0,button_height*5,0);
			if (level[jumpman_s].dontrot & (1 << c)) {
				glBegin(GL_QUADS);
				  Color3f(1.0, 0.0, 0.0);
			} else if (level[jumpman_s].rots & (1 << c)) {
				glBegin(GL_QUADS);
				  Color3f(1.0, 1.0, 0);
			} else {
				glBegin(GL_LINE_LOOP);
				glColor4f(0.5,0.5,0.5,0.5);
			}
			for(int d = 0; d < 4; d++) {				
				glVertex2f(button_verts[d].x, button_verts[d].y);
			}
			glEnd();
			glPopMatrix();
		}
	
		glEnable(GL_DEPTH_TEST);
		goPerspective();
	}
	
	double fps2 = FPS / 2.0;
		
	if (optAngle && ENothing == edit_mode && ticks - fps2 < anglingSince && jumpman_s < level.size()) { // TODO: Display at other times?
		glLoadIdentity();
		goOrtho();
		if (cam_fixed == level[jumpman_s].camera) {
			glRotatef(level[jumpman_s].staticBody->a/M_PI*180,0,0,1);
		} else { // CODE DUPLICATION!
			if (pantype != pan_deep) {
				glRotatef(-roten/M_PI * 180.0,0,0,1);
			} else {
				double nroten = frot(0); // round to something reasonable
				glRotatef(panfade(nroten, surplusSpin)/M_PI * 180.0,0,0,1);
			}
		}

		glDisable(GL_DEPTH_TEST);
	
		double fade = ticks - anglingSince; fade /= fps2; fade /= 2;
	
		// DRAW HERE
		for(int c = 0; c < 8; c++) {
			glPushMatrix();
			glRotatef(180 + 45*c,0,0,1);
			glTranslatef(0,button_height*5,0);
			if (level[jumpman_s].orots == 0 || level[jumpman_s].orots == 1 || level[jumpman_s].odontrot & (1 << c)) {
				glBegin(GL_QUADS);
				  glColor4f(1.0, 0.0, 0.0, 0.5-fade);
			} else if (level[jumpman_s].orots & (1 << c)) {
				glBegin(GL_QUADS);
				  glColor4f(1.0, 1.0, 0, 0.5-fade);
			} else {
				glBegin(GL_LINE_LOOP);
				glColor4f(0.5,0.5,0.5,(0.5-fade)/2);
			}
			for(int d = 0; d < 4; d++) {				
				glVertex2f(scrollbutton_verts[d].x, scrollbutton_verts[d].y);
			}
			glEnd();
			glPopMatrix();
		}
	
		glEnable(GL_DEPTH_TEST);
		goPerspective();
	}

	if (edit_mode == EWalls && drawHelp) {
		glLoadIdentity();
		goOrtho();
		glDisable(GL_DEPTH_TEST);

#ifdef __APPLE__
#define HELPLINES 6
#else
#define HELPLINES 4
#endif
		char *helps[HELPLINES] = 
		{ "Click to draw | Right-click to erase",
		  "Middle-click and drag to pan | Mousewheel to zoom",
#ifdef __APPLE__
		  "(Or: command-click, option-drag, option-up, option-down)",
		  "", 
#endif
		  "F5: Show/hide this help | F6: Show/hide tools | F7: Show/hide grid",
		  "F8: Pause/unpause | F4: Screenshot | ESC: Exit" };

			Color3f(1.0,1.0,1.0);
//			glTranslatef(floatx + floatdx*floatc, floaty, 0);
	//		glTranslatef(chassis->p.x, chassis->p.y, 0);
			glEnable( GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, ftex);
			glTranslatef(0, (button_height)*4.0, 0);
//			if (drawPalette) // This just doesn't work very well. The walls cover up the text, and the spacing's not quite right anyway.
//				glTranslatef(button_width/2 /*+ aspect*(750/2- 270.0)/2*/, 0, 0); // This is probably totally wrong. Whatever
			int d = 0;
			for(list<string>::iterator b = toolHelp.begin(); b != toolHelp.end(); b++) {
				glPushMatrix();
				glTranslatef(-centerOff((*b).c_str()), -floatHeight*(d++ - 2/2.0), 0);
				floating->Render((*b).c_str());
				glPopMatrix();
			}
			
			glTranslatef(0, -(button_height)*8.0
#ifdef __APPLE__
			+ floatHeight
#endif
, 0);

			for(int c = 0; c < HELPLINES; c++) {
				glPushMatrix();
				glTranslatef(-centerOff(helps[c]), -floatHeight*(c - HELPLINES/2.0), 0);
				floating->Render(helps[c]);
				glPopMatrix();
			}
			glDisable( GL_TEXTURE_2D); 
		
		glEnable(GL_DEPTH_TEST);
		goPerspective();
	}
}

	 if (drawMph || drawPalette || drawControls || drawingFloaters.size() > 0) {
		glLoadIdentity();
		goOrtho();
		glDisable(GL_DEPTH_TEST);
		
		if (drawPalette)
			cpSpaceHashEach(workingUiSpace()->staticShapes, &drawButton, NULL);

		if (drawingFloaters.size() > 0) {
			list<floater>::iterator b = drawingFloaters.begin();
			while(b != drawingFloaters.end()) {
				float alpha = 1.0;
				if ((*b).c >= (*b).t-FPS) {
					alpha = ((double)(*b).t-(*b).c)/FPS;
				} else if ((*b).c < FPS) {
					alpha = ((double)(*b).c)/FPS;
				}
				glColor4f((*b).r,(*b).g,(*b).b,alpha);
				glLoadIdentity();
				glTranslatef((*b).x + (*b).dx*(*b).c, (*b).y, 0);
		//		glTranslatef(chassis->p.x, chassis->p.y, 0);
				glEnable( GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, ftex);
				floating->Render((*b).text.c_str());
				glDisable( GL_TEXTURE_2D); 
				(*b).c++;
				
				if ((*b).c > (*b).t) {
					if ((*b).lockout = lockoutHeavy)
						floatheavy = false;
					b = drawingFloaters.erase(b); // Iterates b, incidentally
					resetFloater(); // Might not do anything
				} else {
					b++;
				}
			}
		}
		
		if (drawMph && !doingControlsScreen) {
			char filename[FILENAMESIZE];
#if SELF_EDIT
			if (doingEnding)
				snprintf(filename, FILENAMESIZE, "%d bar %d mode %d", endingAt, endingAtBar, endingAtMode);				
			else
#endif
				snprintf(filename, FILENAMESIZE, "%f sps", cpvlength(chassis->v)/36);
				
			if (haveDoneBlue)
				glColor3f(0.0,0.0,0.0);
			else
				glColor3f(1.0,1.0,1.0);
				
			glLoadIdentity();
			glTranslatef(-centerOff(filename), -750.0/2+2*floatHeight, 0);
			glEnable( GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, ftex);
			floating->Render(filename);
			glDisable( GL_TEXTURE_2D); 

		}
		
		if (drawControls) {
			glLoadIdentity();
			glEnable( GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, ftex);
			glTranslatef(-(280.0+20) / aspect / 2, 0, 0);
			controlsDraw(true, false, false, -1, -1);
			glDisable( GL_TEXTURE_2D); 
		}
		
		glEnable(GL_DEPTH_TEST);
		goPerspective();
	}
	
#if VSYNC_DESPERATION	
	glFlush();
	glFinish();
#endif
	
	SDL_GL_SwapBuffers();
	ticks++;

#if 0
		// Some way to figure out if this is efficient?
	if (optSplatter) { // Cuz full screen double buffering BREAKS EVERYTHING
		glReadBuffer (GL_FRONT);
		glCopyPixels (0,0, surfacew, surfaceh, GL_COLOR);
	}
#endif
	
	moonBuggy_update();
	
#if defined(__APPLE__) && !IN_A_WINDOW
	if (minimizeWant) {
		minimizeWant = false;
		SDL_WM_IconifyWindow();
	}		
#endif
}

void calculateCoordinates() {
	GLdouble model_view[16];
	GLdouble projection[16];
	GLint viewport[4];
	GLdouble tempZ;
				
	goOrtho();	
	glGetDoublev(GL_MODELVIEW_MATRIX, model_view);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluProject(0, 0, 1,
		model_view, projection, viewport,
		&originOrthX, &originOrthY, &tempZ);
	
	gluProject(1, 1, 1,
		model_view, projection, viewport,
		&oneOrthX, &oneOrthY, &tempZ);
		
	oneOrthX -= originOrthX; oneOrthY -= originOrthY;
	
	int old_jumpman_r = jumpman_r;
	jumpman_r = 1;
	goPerspective(); // Make sure jumpman_r is at its defaults when we do this
	jumpman_r = old_jumpman_r;
	
	glGetDoublev(GL_MODELVIEW_MATRIX, model_view);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluProject(0, 0, 1,
		model_view, projection, viewport,
		&originPerX, &originPerY, &tempZ);
	
	gluProject(1, 1, 1,
		model_view, projection, viewport,
		&onePerX, &onePerY, &tempZ);
		
	onePerX -= originPerX; onePerY -= originPerY;

	ERR("Ortho: [%lf, %lf] + [%lf, %lf]. Perspective: [%lf, %lf] + [%lf, %lf].\n", originOrthX, originOrthY, oneOrthX, oneOrthY, originPerX, originPerY, onePerX, onePerY);

}

/*
void timercall(int value)
{
	glutTimerFunc(SLEEP_TICKS, timercall, 0);
		
	glutPostRedisplay();
}
*/
#if FOG_EXPERIMENTAL
extern ContainerBase *container[5];
int fogMode = 0;
double fogDensity = 1.0;
void initFog();

class FogSettingOne : public WheelControl {public:
	FogSettingOne()
		: WheelControl(0,0,2,1) {}
	void feedback() { fogMode = is; if (fogMode > 2) fogMode = 0; if (fogMode < 0) fogMode = 0; initFog(); } 
	virtual void wheel(int dir) { WheelControl::wheel(dir); feedback(); }
	virtual void loseFocus() {
		WheelControl::loseFocus();
		feedback();
	}
};
class FogSettingTwo : public WheelControl {public:
	FogSettingTwo()
		: WheelControl(1,-10,10,0.1) {}
	void feedback() { fogDensity = is; initFog(); } 
	virtual void wheel(int dir) { WheelControl::wheel(dir); feedback(); }
	virtual void loseFocus() {
		WheelControl::loseFocus();
		feedback();
	}
};
#endif

void initFog() {
	int gap = howDeepVisible();
#if FOG_EXPERIMENTAL
	int modes[3] = {GL_LINEAR, GL_EXP, GL_EXP2};

	glFogi(GL_FOG_MODE, modes[fogMode]);
	glFogf(GL_FOG_DENSITY, fogDensity);
#else 
	glFogi(GL_FOG_MODE, GL_LINEAR);
#endif
	glFogf(GL_FOG_START, 1.01);
	glFogf(GL_FOG_END, 1.0f + gap);
}

void initGLBasic()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);

	glPointSize(3.0);
	
//    glEnable(GL_LINE_SMOOTH);
//	glEnable(GL_POINT_SMOOTH);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
//    glHint(GL_POINT_SMOOTH_HINT, GL_DONT_CARE);
//    glLineWidth(1.5);

	goPerspective();
	calculateCoordinates();
	
	glEnable(GL_DEPTH_TEST);
	{
		float FogCol[3]={0.0f,0.0f,0.0f}; // Define a nice light grey
		glFogfv(GL_FOG_COLOR,FogCol);     // Set the fog color
	}
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR); // Note the 'i' after glFog - the GL_LINEAR constant is an integer.
	initFog();
	
	glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
void initFont(int size) {
	floating->FaceSize(size);
	floatHeight = floating->LineHeight();
	
	glGenTextures(1, &ftex);
    glBindTexture(GL_TEXTURE_2D, ftex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4, 4, 0, GL_RGB, GL_FLOAT, texture);
}
void initGL(void)
{
	initGLBasic();

	if (floating)
		delete floating;

	char fontfile[FILENAMESIZE];
	internalPath(fontfile, "DroidSans.ttf");
	floating = new FTGLTextureFont( fontfile);
	
	if ( !floating ){
	  REALERR("Couldn't open critical file: %s\n", fontfile);
	  Quit(1);
	}

	if ( floating->Error() ){
	  REALERR("Couldn't open critical file (error %d): %s\n", (int)floating->Error(), fontfile);
	  Quit(1);
	}
	
	initFont(18);
	
	tempdl[0] = glGenLists(2);
	tempdl[1] = tempdl[0]+1;
}

void glutStuff(int argc, const char *argv[])
{

//	glutInit(&argc, (char**)argv); // Who needs this anyway
	
//	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA);
	
//	glutInitWindowSize(640, 480);
//	glutCreateWindow("Controls: A, D, space. Q to quit.");
	initGL();

	moonBuggy_init();
		
/*
	glutDisplayFunc(display);
	glutKeyboardFunc(moonBuggy_keydown);
	glutKeyboardUpFunc(moonBuggy_keyup);
//	glutMouseFunc(moonBuggy_input);
//	glutIdleFunc(idle);
	glutTimerFunc(SLEEP_TICKS, timercall, 0);
//	glutMouseFunc(buttons);
//	glutPassiveMotionFunc(mouse);
	
	glutMainLoop();
*/
}

inline int downButtonTo0_2(int button) {
	switch (button) {
		case SDL_BUTTON_LEFT:
			return 0;
		case SDL_BUTTON_MIDDLE:
			return 1;
		case SDL_BUTTON_RIGHT:
			return 2;
		default:
			return 3;
	}
}

inline int downToButton(bool *down) {
	if (down[0]) {
		return 1;
	} else if (down[1]) {
		return 2;
	} else if (down[2]) {
		return 3;
	} else
		return 0;
}

void Loop() {
	// TODO: DELETE THIS
	wantClearUi = true;
	want = WMainMenu;
	bool down[3] = {false, false, false};
	
	serviceInterface();
	
  while ( 1 ) {
	{ // Todo: It would be nice to be able to dynamically pick a framerate
		sdlTime = SDL_GetTicks();
		int timesince = sdlTime-lastSdlTime;
		if (timesince > TPF) {
			display();
			lastSdlTime = sdlTime;
			videoFrame();
		} else if (timesince < TPF-1) {
			msleep(TPF-1-timesince);
		}
	}

	// Wtf, a switch-based event loop. I feel like I'm back in the seventh grade using Macintosh Toolbox or something.
	// THIS IS 2008, PEOPLE. GET WITH THE PROGRAM. THE PROGRAM USES CALLBACKS.
	{ SDL_Event event;
      while ( SDL_PollEvent(&event) ) {
		clickConnected = false;
	  
		videoEvent(event);
	  
        if ( event.type == SDL_QUIT ) {
          Quit();
        }

		if (doingControlsScreen && controlsScreenProcess(event))
			continue;

#if defined(__APPLE__) && !IN_A_WINDOW
		if (event.type == SDL_ACTIVEEVENT && event.active.gain && event.active.state == SDL_APPACTIVE && !optWindow) {
			recreate_surface(false);
			if (!minimizeAlreadyPaused)
				pause(false);
		} else
#endif
		if (event.type == SDL_KEYDOWN && KeyboardControl::focus != NULL) {
			KeyboardControl::focus->key(event.key.keysym.unicode, event.key.keysym.sym);
		} else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
//			ERR("KEY EVENT TYPE %d KEY %d\n", (int)event.type, (int)event.key.keysym.sym); break;

			if (doingEnding && endingAtMode == 9) {
				endEnding();
				wantClearUi = true; // Code duplication?
				want = WMainMenu;
				serviceInterface();
			}

			int control = -1;
			for(int c = 0; c < NUMCONTROLS; c++) {
				if (controlType[c] == ct_key && controlKeys[c] == event.key.keysym.sym)
					control = c;
			}

#if 0
			if (event.type == SDL_KEYDOWN) {
				int w = -1;
				switch (event.key.keysym.sym) {
					case '0': case '1': case '2': case '3': case '4': case '5':
					case '6': case '7': case '8': case '9':
						w = event.key.keysym.sym - '0';
						if (w == 0) w += 10;
						break;
					case 'q': w = 11; break;
					case 'w': w = 12; break;
					case 'e': w = 13; break;
					case 'r': w = 14; break;
					case 't': w = 15; break;
					case 'y': w = 16; break;
					case 'u': w = 17; break;
					case 'i': w = 18; break;
					case 'o': w = 19; break;
					case 'p': w = 20; break;
					case '[': w = 21; break;
					case ']': w = 22; break;
					case 'a': w = 23; break;
					case 's': w = 24; break;
					case 'd': w = 25; break;
					case 'f': w = 26; break;
					case 'g': w = 27; break;
					case 'h': w = 28; break;
					case 'j': w = 29; break;
					case 'k': w = 30; break;
					case 'l': w = 31; break;
					case ';': w = 32; break;
					case '\'': w =33; break;
					case 'z': w = 34; break;
					case 'x': w = 35; break;
					case 'c': w = 36; break;
					case 'v': w = 37; break;
					case 'b': w = 38; break;
					case 'n': w = 39; break;
					case 'm': w = 40; break;
					case ',': w = 41; break;
					case '.': w = 42; break;
					case '/': w = 43; break;
				}
					
				if (w >= 0) {
					w = 12-w;
					double s = pow(pow(2, 1/12.0), w);
					sjump.w = 100;
					sjump.w *= s;
					ERR("w: %d s: %lf w: %d\n", w, s, sbell.w);
					sjump.reset();
				}
			}
#endif

#if defined(__APPLE__)
			SDLMod mods = SDL_GetModState(); // OS X has some issues we have to work around.
#if !IN_A_WINDOW
			if (!optWindow && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_TAB && (mods & KMOD_LMETA)) { // We have to simulate alt-tab working
				recreate_surface(true); // leave fullscreen, become a window
				minimizeAlreadyPaused = paused;
				if (!minimizeAlreadyPaused)
					pause(true);
				minimizeWant = true;
			} else
#endif
			// Also, we need to offer option-up and option-down for when there is no mouse wheel.
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_UP && (mods & KMOD_LALT || mods & KMOD_RALT)) {
				clickLastResortHandle(0, 0, 4); // Zoom in
			} else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_DOWN && (mods & KMOD_LALT || mods & KMOD_RALT)) {
				clickLastResortHandle(0, 0, 5); // Zoom out
			} else 
#endif
			if (control >= 0) {
				if ( event.type == SDL_KEYDOWN ) {
					moonBuggy_keydown(control);
				}
				if ( event.type == SDL_KEYUP ) {
					moonBuggy_keyup(control);
				}
			} else if (/*edit_mode && edit_mode != EPlayground &&*/ event.type == SDL_KEYDOWN) {
				switch ( event.key.keysym.sym ) {
					case SDLK_F5:
						if (edit_mode == EWalls) {
							drawHelp = !drawHelp;
							slick.w = drawHelp ? 1 : 4;
							slick.reset();
						}
						break;
					case SDLK_F6:
						if (edit_mode == EWalls) {
							drawPalette = !drawPalette;
							slick.w = drawPalette ? 1 : 4;
							slick.reset();
						}
						break;
					case SDLK_F7:
						if (edit_mode == EWalls) {
							drawGrid = !drawGrid;
							slick.w = drawGrid ? 1 : 4;
							slick.reset();
						}
						break;
					case SDLK_F8:
						pause(!paused);
						slick.w = !paused ? 1 : 4;
						slick.reset();
						if (edit_mode == EWalls) {
							justLoadedCleanup();
							reentryEdit();
						}
						break;
					case SDLK_F3: {
						drawMph = !drawMph;
#if FOG_EXPERIMENTAL
						if (!container[CLEFT]) {
							container[CLEFT] = new ContainerBase(uiSpace, CLEFT);
							container[CLEFT]->add(new FogSettingOne());
							container[CLEFT]->add(new FogSettingTwo());
							container[CLEFT]->commit();
						}
#else
#if SELF_EDIT && 0
						startEnding();
//						level[0].zoom = 0.5;
#else
						slick.w = !drawMph ? 1 : 4;
						slick.reset();
#endif
#endif
						} break;
#if DOING_VIDEOS
					case SDLK_F4:
						videoPlayback(false);
						break;
					case SDLK_F13:
						videoPlayback();
						break;
					case SDLK_F14:
						videoSave();
						break;
					case SDLK_F15:
						videoLoad();
						break;
#else
#if 1						
					case SDLK_F4:
						slick.w = 2;
						slick.reset();
						screenshot();
						break;
#else
					case SDLK_F13:
						if (edit_mode == EWalls) { // If not in edit mode: Don't bother
							if (editSlice.size() > 2) { // If there are no layers: Don't bother
								editLayer++;
								editLayer %= (editSlice.size()/2);
								
								sbell.reset();
								chassisShape->layers = (1 << editLayer);
								layerYell();
							} else {
								sland.reset();
							}
						}
						break;
#endif
#if SELF_EDIT
					case SDLK_F14:
						if (jumpman_s == 0) {
							sland.reset();
						} else {
							jumpmanstate = jumpman_wantexit; 
							exit_direction = -1;
						}
						break;
					case SDLK_F15:
						jumpmanstate = jumpman_wantexit; 
						exit_direction = 1;
						break;
#endif
#endif
				}
			}
		}
		
		if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP) { // Again some redundancy etc.
			int control = -1;
			for(int c = 0; c < NUMCONTROLS; c++) {
				if (controlType[c] == ct_pad && controlPad[c] == event.jbutton.button)
					control = c;
			}
			if (control >= 0) {
				if ( event.type == SDL_JOYBUTTONDOWN ) {
					moonBuggy_keydown(control);
				}
				if ( event.type == SDL_JOYBUTTONUP ) {
					moonBuggy_keyup(control);
				}
			}
		}
		
		if (event.type == SDL_JOYAXISMOTION) { // God damn this control code is complicated!
			int status = (((int)event.jaxis.axis) + 1) * (event.jaxis.value > 0 ? 1 : -1); // Joystick code for current event
			if (iabs(event.jaxis.value)<DPAD_THRESHOLD) status = 0;
			int &oldstatus = controlAxes[event.jaxis.axis]; // Last code pressed on this axis
			if (oldstatus && oldstatus != status) {	// If previously a button was pressed, but now it isn't
				for(int c = 0; c < NUMCONTROLS; c++) {
					int control = -1;
					if (controlType[c] == ct_dpad && controlPad[c] == oldstatus)
						control = c;
					if (control >= 0)
						moonBuggy_keyup(control);
				}
			}
			if (status && oldstatus != status) {
				for(int c = 0; c < NUMCONTROLS; c++) { // If no button was pressed previously, but now one is
					int control = -1;
					if (controlType[c] == ct_dpad && controlPad[c] == status)
						control = c;
					if (control >= 0)
						moonBuggy_keydown(control);
				}
			}
			oldstatus = status;
		}
		
		if (event.type == SDL_JOYHATMOTION) {
			int status = event.jhat.value; // Hat "button" for this event
			int &oldstatus = controlHat[event.jhat.hat]; // Last button pressed on this hat
			if (oldstatus != status) { // If hat has changed
				for(int c = 0; c < NUMCONTROLS; c++) {
					if (controlType[c] == ct_hat && hatMatchHat(controlPad[c], event.jhat.hat)) {
						int value = hatValue(controlPad[c]);
						if ((status & value) ^ (oldstatus & value)) { // I.E. if exactly one
							if (oldstatus & value)
								moonBuggy_keyup(c); // Allow us to fire more than one thing per event
							else
								moonBuggy_keydown(c);
						}
					}
				}
			}
			oldstatus = status;
		}
		
		// "Local origin". Take into account instantaneous scaling.
		GLdouble lnePerX = onePerX * scan_r, 
		 lnePerY = onePerY * scan_r,
		 lriginPerX = originPerX + scan_x*lnePerX,
		 lriginPerY = originPerY + scan_y*lnePerY;

		if (event.type == SDL_MOUSEBUTTONDOWN) {
				if (drawPalette) {
					GLdouble orthX = ( event.button.x - originOrthX) / oneOrthX;
					GLdouble orthY = ( event.button.y - originOrthY) / oneOrthY;				
				
					if (!(event.button.button == 2 || event.button.button == 4 || event.button.button == 5))
						cpSpaceStaticShapePointQuery(workingUiSpace(), cpv(orthX, -orthY), clicked, NULL);
					else {
						int dir = 0;
						if (event.button.button == 4) dir = 1;
						if (event.button.button == 5) dir = -1;
						cpSpaceStaticShapePointQuery(workingUiSpace(), cpv(orthX, -orthY), wheeled, (void *)dir);
					}
				}
				
				if (!clickConnected && edit_mode
					// This next case indicates I have failed in making my system here robust.
					&& !(edit_mode != ENothing && down[1] && (event.button.button == 4 || event.button.button == 5)) 				
				) {
					GLdouble perX = ( event.button.x - lriginPerX) / lnePerX;
					GLdouble perY = ( event.button.y - lriginPerY) / lnePerY;
					ERR("CLICK %d [%d, %d] = [%lf, %lf]  --- via: [%lf, %lf] [%lf, %lf]\n", event.button.button, event.button.x, event.button.y, perX, perY, lriginPerX, lriginPerY, lnePerX, lnePerY);
					
					clickLastResortHandle(perX, perY, event.button.button);
					int b03 = downButtonTo0_2(event.button.button);
					if (b03 < 3)
						down[b03] = true;
				}
		}
		
		if (event.type == SDL_MOUSEBUTTONUP) { // Let this run even if edit mode off
			int b03 = downButtonTo0_2(event.button.button);
			if (b03 < 3)
				down[b03] = false;
		}
		
		if (event.type == SDL_MOUSEMOTION && edit_mode) {
			int button = downToButton(down);
//			ERR("MOVING %d=%d [%d, %d]\n", event.motion.state, button, event.button.x, event.button.y);				
			
			if (button) {
				GLdouble perX = ( event.button.x - lriginPerX) / lnePerX;
				GLdouble perY = ( event.button.y - lriginPerY) / lnePerY;

//				ERR("DRAGGING %d=%d [%d, %d] = [%lf, %lf]\n", event.motion.state, button, event.button.x, event.button.y, perX, perY);
				
				clickLastResortHandle(perX, perY, button, true);
			}
		}
		
		if (clickConnected) { // Somewhere above, a control event has occurred.
			serviceInterface();
		}
		
	}
    }
  }
}

void startEnding() {
	if (!doingEnding) {		
		clearEverything();
		dummyStage();
		level[0].space->gravity = cpvzero;	
		
		endingAt = 0;
		endingAtBar = 0;
		endingAtMode = 0;
		endingAtMsg = 0;
		modeStartsAt = 0;
		modeEndsAt = 0;
		glClearColor(0.0, 0.0, 0.0, 0.0);
		onEsc = WEndingExit;
	}
	
	doingEnding = true;
	edit_mode = EPlayground;
}

void endEnding() {
	if (doingEnding) {
		doingEnding = false;
		endingAtMode = 0;
		edit_mode = ENothing;

		clearEverything();
		dummyStage(); // Might as well
		haveDoneBlue = false;
		glClearColor(0.0, 0.0, 0.0, 0.0);
		onEsc = WNothing;
		
		sland.ticks = 0; sland.max = 5000; sland.w = 400; sland.amp = 0.05;
		sbell.ticks = 0; sbell.max = 30000; sbell.w = 200; sbell.amp = 0.05;
		shatter.ticks = 0; shatter.max = 30000; shatter.w = 300; shatter.amp = 0.05;
		slick.ticks = 0; slick.max = 500; slick.w = 1; slick.amp = 0.075;
		
		if (!haveWonGame) {
			addFloater("Splatter mode unlocked");
			haveWonGame = true;
		}
	}
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


int main(int argc, char*argv[]) 
{
	{
		time_t rawtime;
		time ( &rawtime );
		srandom(rawtime);
	}

	cpInitChipmunk();
	uiSpace = cpSpaceNew();  
	hiddenControlsSpace = cpSpaceNew();
	
  if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0 ) {
    REALERR("Unable to initialize SDL: %s\n", SDL_GetError());
    return 1; // WINDOWS VISTA CROSSCOMPILE HATES exit() SO I NO LONGER CALL IT?!?
  }
	
	sdl_init();

	glutStuff(argc, (const char**)argv);
	
	Loop();
	return 0;
}

// ------ THIS LAST BIT HERE JUST HANDLES THE CONTROLS SCREEN. I GUESS I OUGHT TO PUT IT IN A SEPARATE FILE EVENTUALLY.

char *readableVersion(SDLKey key) {
	if (key >= '!' && key <= '~') {
		static char letter[2];
		letter[0] = key;
		letter[1] = '\0'; 
		return letter;
	}
	switch (key) {
		case ' ':
			return "[Space]";
		case SDLK_ESCAPE:
			return "[ESC]";
		case SDLK_BACKSPACE:
			return "[Delete]";
		case SDLK_TAB:
			return "[Tab]";
		case SDLK_RETURN:
			return "[Return]";
		case SDLK_KP0:
			return "[Keypad 0]";
		case SDLK_KP1:
			return "[Keypad 1]";
		case SDLK_KP2:
			return "[Keypad 2]";
		case SDLK_KP3:
			return "[Keypad 3]";
		case SDLK_KP4:
			return "[Keypad 4]";
		case SDLK_KP5:
			return "[Keypad 5]";
		case SDLK_KP6:
			return "[Keypad 6]";
		case SDLK_KP7:
			return "[Keypad 7]";
		case SDLK_KP8:
			return "[Keypad 8]";
		case SDLK_KP9:
			return "[Keypad 9]";
		case SDLK_UP:
			return "[Up arrow]";
		case SDLK_DOWN:
			return "[Down arrow]";
		case SDLK_RIGHT:
			return "[Right arrow]";
		case SDLK_LEFT:
			return "[Left arrow]";
		case SDLK_F1:
			return "[F1]";
		case SDLK_F2:
			return "[F2]";
		case SDLK_F3:
			return "[F3]";
		case SDLK_F4:
			return "[F4]";
		case SDLK_F5:
			return "[F5]";
		case SDLK_F6:
			return "[F6]";
		case SDLK_F7:
			return "[F7]";
		case SDLK_F8:
			return "[F8]";
		case SDLK_F9:
			return "[F9]";
		case SDLK_F10:
			return "[F10]";
		case SDLK_F11:
			return "[F11]";
		case SDLK_F12:
			return "[F12]";
		case SDLK_F13:
			return "[F13]";
		case SDLK_F14:
			return "[F14]";
		case SDLK_F15:
			return "[F15]";
		case SDLK_RSHIFT:
			return "[Right shift]";
		case SDLK_LSHIFT:
			return "[Left shift]";
		case SDLK_RCTRL:
			return "[Right control]";
		case SDLK_LCTRL:
			return "[Control]";
#if defined(__APPLE__)
		case SDLK_RALT:
			return "[Right option]";
		case SDLK_LALT:
			return "[Option]";
		case SDLK_RMETA:
			return "[Right command]";
		case SDLK_LMETA:
			return "[Command]";
		case SDLK_RSUPER:
			return "[Right Windows (?!)]"; // Eww
		case SDLK_LSUPER:
			return "[Windows (?!)]";
#else
		case SDLK_RALT:
			return "[Right alt]";
		case SDLK_LALT:
			return "[Alt]";
		case SDLK_RMETA:
			return "[Right meta]";
		case SDLK_LMETA:
			return "[Meta]";
		case SDLK_RSUPER:
			return "[Right Windows]";
		case SDLK_LSUPER:
			return "[Windows]";
#endif
		case SDLK_CAPSLOCK:
			return "[Caps lock]";
		case SDLK_KP_ENTER:
			return "[Enter]";
			
		default:
			return "[Something]";
	}
}
char *readableVersionPad(Uint8 button) {
	static char letter[FILENAMESIZE];
	snprintf(letter, FILENAMESIZE, "[%s %d]", SDL_JoystickName(0), (int)button);
	return letter;
}

char *readableVersionDpad(int button) {
	static char letter[FILENAMESIZE];
	snprintf(letter, FILENAMESIZE, "[%s axis %d %s]", SDL_JoystickName(0), iabs(button)-1, button>0?"+":"-");
	return letter;
}

char *readableVersionHat(int button) {
	static char letter[FILENAMESIZE];
	char *dirname;
	int value = hatValue(button);
	int hat = hatHat(button);
	switch (value) {
		case SDL_HAT_UP: dirname = "up"; break;
		case SDL_HAT_DOWN: dirname = "down"; break;
		case SDL_HAT_LEFT: dirname = "left"; break;
		case SDL_HAT_RIGHT: dirname = "right"; break;
		default: dirname = "???"; break;
	}
	snprintf(letter, FILENAMESIZE, "[%s hat %d %s]", SDL_JoystickName(0), hat, dirname);
	return letter;
}

bool controlSame(int c1, int c2) {
	if (controlType[c1] != controlType[c2])
		return false;
	switch(controlType[c1]) {
		case ct_key:
			return controlKeys[c1] == controlKeys[c2];
		case ct_pad: case ct_dpad: case ct_hat:
			return controlPad[c1] == controlPad[c2];
		default:
			return false;
	}
}

char *readableControl(int c) {
			switch (controlType[c]) {
				case ct_key:
					 return readableVersion(controlKeys[c]);
					break;
				case ct_pad:
					return readableVersionPad(controlPad[c]);
					break;
				case ct_dpad:
					return readableVersionDpad(controlPad[c]);
					break;
				case ct_hat:
					return readableVersionHat(controlPad[c]);
					break;
				default:
					return "[Nothing]";
			}
}

class OneControlControl : public KeyboardControl
{
public:
	static int selectedControl;
	KeyboardControl *next;
	string title;
	int select;
	void changedKey();
	OneControlControl(string _title = string(), int _select = -1, bool active = true, KeyboardControl *_next = NULL)
		: KeyboardControl("", false, active), next(_next), title(_title), select(_select) { changedKey(); }
	virtual void key(Uint16 unicode, SDLKey key);
	virtual void click() { if (bg) KeyboardControl::click(); }
	virtual void loseFocus() { selectedControl = -1; KeyboardControl::loseFocus(); }
	virtual void takeFocus() { KeyboardControl::takeFocus(); selectedControl = select; }
};

int OneControlControl::selectedControl = -1;

OneControlControl *controlControls[NUMEDITCONTROLS];
int lastjoyaxisstatus = 0;

void OneControlControl::changedKey() {
	text = title;
	text += ": ";
	text += readableControl(select);
}

void OneControlControl::key(Uint16 unicode, SDLKey key) {
	if (key == SDLK_ESCAPE) {
		loseFocus();
		return;
	}
}

bool controlsScreenProcess(SDL_Event &event) {
	doingControlsScreen; OneControlControl::selectedControl;
	bool eat = false;
	bool moveOn = false;
	bool anythingSelected = OneControlControl::selectedControl >= 0;
	int desiredjoyaxisstatus = 0;
	
	ControlType oldControlType = controlType[OneControlControl::selectedControl];
	SDLKey oldControlKey = controlKeys[OneControlControl::selectedControl];
	int oldControlPad = controlPad[OneControlControl::selectedControl];	
	
	if ( event.type == SDL_KEYDOWN ) {
		if (event.key.keysym.sym == controlKeys[CONTROLS]) {
			controlControls[0]->takeFocus();
			eat = true;
		} else if (anythingSelected) {
			if (event.key.keysym.sym == controlKeys[QUIT]) {
				if (KeyboardControl::focus)
					KeyboardControl::focus->loseFocus();
			} else {
				controlType[OneControlControl::selectedControl] = ct_key;
				controlKeys[OneControlControl::selectedControl] = event.key.keysym.sym;
				moveOn = true;
			}
			eat = true;
		}
	}
	if ( anythingSelected && event.type == SDL_JOYBUTTONDOWN ) {
		controlType[OneControlControl::selectedControl] = ct_pad;
		controlPad[OneControlControl::selectedControl] = event.jbutton.button;
		moveOn = true;
	
#if AEOLIST_DEBUG
		{ 	char filename2[FILENAMESIZE];
			snprintf(filename2, FILENAMESIZE, "Button %d", (int)event.jbutton.button);
			debugYell(filename2);
		}
#endif
	}
	if (anythingSelected && event.type == SDL_JOYAXISMOTION) {
		int status = (((int)event.jaxis.axis) + 1) * (event.jaxis.value > 0 ? 1 : -1);

		if (iabs(event.jaxis.value)>=DPAD_THRESHOLD && status != lastjoyaxisstatus) {
			controlType[OneControlControl::selectedControl] = ct_dpad;
			controlPad[OneControlControl::selectedControl] = status;
			desiredjoyaxisstatus = status;
			moveOn = true;
		}
		
#if AEOLIST_DEBUG
		{ 	char filename2[FILENAMESIZE];
			snprintf(filename2, FILENAMESIZE, "Axis %d = %d%s", (int)event.jaxis.axis, (int)event.jaxis.value, moveOn?"!":"");
			debugYell(filename2);
		}
#endif
	}
	if (anythingSelected && event.type == SDL_JOYHATMOTION) {
		int status = hatStatus(event.jhat.hat, event.jhat.value);

		if (event.jhat.value == SDL_HAT_LEFT || event.jhat.value == SDL_HAT_RIGHT // Only NEWS matter for these purposes
		 || event.jhat.value == SDL_HAT_UP || event.jhat.value == SDL_HAT_DOWN) {
			controlType[OneControlControl::selectedControl] = ct_hat;
			controlPad[OneControlControl::selectedControl] = status;
			moveOn = true;
		}
		
#if AEOLIST_DEBUG
		{ 	char filename2[FILENAMESIZE];
			snprintf(filename2, FILENAMESIZE, "Hat %d = %d%s", (int)event.jhat.hat, (int)event.jhat.value, moveOn?"!":"");
			debugYell(filename2);
		}
#endif
	}
	if (moveOn) {	
		sjump.reset();
		for(int c = 0; c < NUMEDITCONTROLS; c++) { // Swap on duplicate
			if (c != OneControlControl::selectedControl && controlSame(c, OneControlControl::selectedControl)) {
				controlType[c] = oldControlType;
				controlKeys[c] = oldControlKey;
				controlPad[c] = oldControlPad;
				controlControls[c]->changedKey();
				break;
			}
		}
		lastjoyaxisstatus = desiredjoyaxisstatus;
	
		OneControlControl *selected = controlControls[OneControlControl::selectedControl];
		selected->changedKey();
		if (selected->next)
			selected->next->takeFocus();
		else
			selected->loseFocus();
	}	
	return eat;
}

void populateControlsScreen(ContainerBase *intoContainer, string mention) {
	OneControlControl *last = NULL;
	intoContainer->add( new ControlBase ("Controls") );
	if (!mention.empty())
		intoContainer->add( new ControlBase(mention) );
	{
		string joystick;
		if (gotJoystick) {
			joystick = "Using gamepad: ";
			joystick += SDL_JoystickName(0);
		}
		intoContainer->add( new ControlBase(joystick) );
	}
	intoContainer->add( new ControlBase ("") );
	for (int c = NUMCONTROLS-1; c >=0; c--) {
		last = new OneControlControl(controlNames[c], c, c<NUMEDITCONTROLS, c<NUMEDITCONTROLS-1 ? last : NULL);
		controlControls[c] = last;
	}	
	for(int c = 0; c < NUMCONTROLS; c++) {
		if (c == NUMEDITCONTROLS)
			intoContainer->add( new ControlBase ("") );
		intoContainer->add( controlControls[c] );
	}
	doingControlsScreen = true;
	OneControlControl::selectedControl = -1;
	lastjoyaxisstatus = 0;
}

void depopulateControlsScreen() {
	doingControlsScreen = false;
}

class ControlsDoneButton : public ControlBase {
public:
	ControlsDoneButton(string _text) : ControlBase(_text, true) {
	}
	virtual void click() {
		sjump.reset();
		destroyControlsScreen();
	}
};

void controlsScreen(bool startdone) {		
	if (doingControlsScreen)
		return;

	doingControlsScreen = true;
	doingHiddenControlsScreen = true;
	completelyHalted = true;
	
	controlsScreenAlreadyPaused = paused;
	if (!controlsScreenAlreadyPaused)
		pause(true);
	
	hiddenControlsScreen[1] = new ContainerBase(hiddenControlsSpace, CMID);
	for(int c = 0; c < 6; c++)
		hiddenControlsScreen[1]->add( new ControlBase( "" ) );
	hiddenControlsScreen[1]->add( new ControlsDoneButton( "Done" ) );
	hiddenControlsScreen[1]->commit();
	
	hiddenControlsScreen[0] = new SquishContainer(hiddenControlsSpace, CMID);
	populateControlsScreen(hiddenControlsScreen[0], "(Click to change)");
	for(int c = 0; c < 2; c++)
		hiddenControlsScreen[0]->add( new ControlBase( "" ) );
	hiddenControlsScreen[0]->commit();
} 

void destroyControlsScreen() {
	delete hiddenControlsScreen[0];
	delete hiddenControlsScreen[1];
	doingControlsScreen = false;
	doingHiddenControlsScreen = false;
	completelyHalted = false;
	
	if (!controlsScreenAlreadyPaused)
		pause(false);
}
	
void BombBox(string why) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	goOrtho();

	float leftMargin = -centerOff("(Press a key or gamepad button to set the highlighted control)");
	int lineCount = -1;

	glColor3f(1.0, 0.5, 0.0);
	
	glPushMatrix();
	glTranslatef(leftMargin-72, 200, 0);
	glRotatef(180,0,1,0);
//	glScalef(1.5, 1.5, 1.0);
	
	if (pinfo[C_BOMB])
		glCallList(pinfo[C_BOMB]->displays[0]);
	glPopMatrix();

	glEnable( GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ftex);
	glPushMatrix();
	
	glTranslatef(leftMargin, 200, 0);
	
	glPushMatrix();
		floating->Render("EVERYTHING IS BROKEN");
	glPopMatrix();
	glTranslatef(0, -floatHeight, 0);
	glTranslatef(0, -floatHeight, 0);
	while (!why.empty()) { // Wraps text. I can't believe I have to do simple stuff like this myself..
		char filename2[FILENAMESIZE+1]; memset(filename2, 0, FILENAMESIZE+1);
		int size = why.size(); if (size>FILENAMESIZE) size = FILENAMESIZE;
		int c = 0;
		for(; c < size; c++) {
			char next = why[c];
			if ('\n' == next) { 
				if (c > 0 && c+1 == why.size()) // Special case: '\n' comes at end of string
					c--;
				break;
			}
			filename2[c] = next;
			if (c > 0 && -centerOff(filename2) < leftMargin-36) {
				filename2[c] = '\0';
				c--;
				break;
			}
		}
		glPushMatrix();
			floating->Render(filename2);
		glPopMatrix();
		glTranslatef(0, -floatHeight, 0);
		lineCount++;
		
		why.replace(0, c+1, "");
	}
	glPushMatrix();
		floating->Render("The program must shut down.");
	glPopMatrix();
	glTranslatef(0, -floatHeight, 0);
	glTranslatef(0, -floatHeight, 0);
	glPushMatrix();
		floating->Render("Press any key to continue.");
	glPopMatrix();

	glDisable( GL_TEXTURE_2D); 
	glPopMatrix();
	
	glBegin(GL_LINE_LOOP);
	glVertex2f( leftMargin-72*2, 200+72);
	glVertex2f(-leftMargin+72*2, 200+72);
	glVertex2f(-leftMargin+72*2, 100-72 - lineCount*floatHeight);
	glVertex2f( leftMargin-72*2, 100-72 - lineCount*floatHeight);
	glEnd();

	SDL_GL_SwapBuffers();		

	while(1) { SDL_Event event;
		while ( SDL_PollEvent(&event) ) {
		//		ERR("OK event %d (vs %d) ... axis %d value %d\n", (int) event.type, (int)SDL_JOYAXISMOTION, (int)event.jaxis.axis, (int)event.jaxis.value);
			if ( event.type == SDL_QUIT || event.type == SDL_KEYDOWN) {
			  Quit(1);
			}
		}
	}
}

void FileBombBox(string filename) {
	string because;
	if (filename.empty())
		because = "An internal file could not be opened.";
	else
		because = "Could not open file:\n";
	BombBox(because + filename + "\n");
}

#include "video.h"
