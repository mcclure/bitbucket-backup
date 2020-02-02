#ifndef _JUMPCORE_DISPLAY
#define _JUMPCORE_DISPLAY

#include <vector>
#include "chipmunk.h"
#include "slice.h"

// Note: DRAW_DEBUG will likely break on iPhone or other ES platforms. And I'm not sure about GL2 mode.
// I'm not fixing this, it's for debugging anyway?
#define DRAW_DEBUG 0

#define WHITE 0xFFFFFFFF
#define BLACK 0xFF000000

// Util

void drawObject(void *ptr, void *data); // For chipmunk
cpVect screenToGL(int screenx, int screeny, double desiredZ);

inline unsigned int packColor(float r, float g, float b, float a = 1) {
	unsigned int color = 0;
	unsigned char c;
	c = a*255; color |= c; color <<= 8;
	c = b*255; color |= c; color <<= 8;
	c = g*255; color |= c; color <<= 8;
	c = r*255; color |= c;
	return color;
}

inline unsigned int packGray(float g, float a = 1) {
	return packColor(g,g,g,a);
}

unsigned int packHsv(float h, float s, float v, float a = 1);
unsigned int randomColor();
unsigned int randomGray();

static inline cpVect cpvscale(const cpVect v1, const cpVect v2) {
	return cpv(v1.x * v2.x, v1.y * v2.y);
}

static inline cpVect ynorm(const cpVect v1) {
	return cpvmult(v1, 1.0/v1.y);
}

extern double aspect;
extern int surfacew, surfaceh;

void goOrtho();
void orthoText();
void goPerspective();

// FIXME: Reorganize stuff so putting the include way down here isn't necessary.
#include "pile.h"

#endif /* _JUMPCORE_DISPLAY */