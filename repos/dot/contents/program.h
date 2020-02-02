/*
 *  program.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 4/27/08.
 *
 */

#include "kludge.h"
#include <math.h>
#include <string>
#include <list>
#include <vector>
#include "cpVect.h"

using namespace std;

#define NAME_OF_THIS_PROGRAM "."

#define IN_A_WINDOW 1

// The following handful of lines are the only ones you can't erase. Feel free to redefine "FPS".

#define FPS 80.0
#define TPF ((int)(1000/FPS))
extern int ticks;
extern int sinceLastFrame;
extern double aspect;
extern bool gotJoystick;
extern bool paused;
extern int surfacew, surfaceh;
cpVect screenToGL(int screenx, int screeny, double desiredZ);

void recreate_surface(bool in_a_window);

// CALLBACKS

void BackOut();
void AboutToQuit();
void Quit(int code = 0);
void BombBox(string why = string());
void FileBombBox(string filename = string());

void program_init(void);
void program_reset(void);
void program_update(void);
void program_eventkey(SDL_Event &event);
void program_eventjoy(SDL_Event &event);
void program_eventmouse(SDL_Event &event);

void display(void);
void display_init();
void drawButton(void *ptr, void *data);
void audio_callback(void *userdata, uint8_t *stream, int len);

// Interface / ControlBase

extern bool &optWindow;
inline int iabs(int i) { return i < 0 ? -i : i; } // Yeah, I dunno.

void goOrtho();

float textWidth(string str);
float textHeight(string str);

// Should quadpile be visible outside display.cpp in the first place?

template <typename T> inline T xe(T x, int d, T i = 1) {
    switch(d) {
        case 1:
            return (x+i);
        case 3:
            return x-i;
        default: return x;
    }
}

template <typename T> inline T ye(T y, int d, T i = 1) {
    switch (d) {
        case 0:
            return y-i;
        case 2:
            return y+i;
        default:
            return y;
    }
}

inline int un(int d) {
    return (d+2)%4;
}

struct quadpile : public vector<float> {
	static vector<unsigned short> megaIndex;
	static void megaIndexEnsure(int count);
	void push(float x1, float y1, float x2, float y2, uint32_t packedColor);
	void push(float x1, float y1, float x2, float y2, float r, float g, float b, float a = 1.0);
};

static unsigned int packColor(float r, float g, float b, float a = 1) {
	unsigned int color = 0;
	unsigned char c;
	c = a*255; color |= c; color <<= 8;
	c = b*255; color |= c; color <<= 8;
	c = g*255; color |= c; color <<= 8;
	c = r*255; color |= c;
	return color;
}

// State for .

struct thing;

struct square_id {
	const char *id;
	int x, y;
	square_id() : id(0),x(-1),y(-1) {}
	square_id(const char *_id, int _x, int _y) : id(_id),x(_x),y(_y) {}
};

struct square {
    square *nesw[4];
    list<thing *> anchor;
#if NAMETRACK
	square_id id; // Parent slice
#endif
	
    bool solid:1, blocked:1;
    virtual void draw(int x, int y) {}
    virtual ~square() {}
    square(square_id *_id, bool _solid = false) : solid(_solid), blocked(false) { 
		for(int c = 0; c < 4; c++) nesw[c] = NULL; 
#if NAMETRACK
		if (_id) id = *_id;
#endif
	}
};

struct stitch_square : public square { // Should all be gone by the time drawing happens
    square *partner;
    stitch_square(square_id *_id) : square(_id), partner(NULL) {}
};

struct plain_square : public square {
    uint32_t color;
    virtual void draw(int x, int y);
    plain_square(square_id *_id, uint32_t _color, bool _solid = false) : square(_id, _solid), color(_color) {}
};

struct thing {
    square *anchored;
    virtual void draw(int x, int y) = 0;
    virtual void act() = 0;
    virtual void attach(square &to);
    virtual ~thing() {}
    thing() : anchored(NULL) {}
};

struct dot_thing : public thing {
    uint8_t md, mv, mt; // Motion direction, motion "velocity", motion tick
    bool can(int d);

    uint32_t color;
    cpVect offset;
    virtual void draw(int x, int y);
    virtual void act();
    void reanchor();
    dot_thing(uint32_t _color = 0x00000000) : thing(), color(_color), offset(cpvzero), md(0), mv(0), mt(0) {}
};

struct chassis_thing : public dot_thing {
    virtual void act();
    chassis_thing() : dot_thing(ntohl(0xFF0000FF)) { }
};

struct bystander_thing : public dot_thing {
    virtual void act();
    bystander_thing(uint32_t _color) : dot_thing(_color) { }
};

extern chassis_thing chassis;
extern vector<thing *> acting;

void watch_file(const string &filename);