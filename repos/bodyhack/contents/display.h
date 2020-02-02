#ifndef _JUMPCORE_DISPLAY
#define _JUMPCORE_DISPLAY

#include <vector>
#include "chipmunk.h"
#include "slice.h"

// Note: DRAW_DEBUG will likely break on iPhone or other ES platforms. And I'm not sure about GL2 mode.
// I'm not fixing this, it's for debugging anyway?
#define DRAW_DEBUG 0

void text_init();
void drawText(string text, double x, double y, double rot = 0, bool xcenter = true, bool ycenter = true);

// Vertexes, then color, then texture
#define VERT_STRIDE 2
#define COLOR_STRIDE 1
#define TEX_STRIDE 2

#define WHITE 0xFFFFFFFF

// Util

void drawObject(void *ptr, void *data); // For chipmunk
cpVect screenToGL(int screenx, int screeny, double desiredZ);

struct stateoid;

struct quadpile : public vector<float> {
	static vector<unsigned short> megaIndex;
	bool useTexture, useColor; GLint texture; unsigned int baseColor;
	stateoid *stateSetter;
	quadpile(bool _useTexture = true, bool _useColor = false, GLint _texture = 0, unsigned int _baseColor = WHITE)
		: vector<float>(), useTexture(_useTexture), useColor(_useColor), texture(_texture), baseColor(_baseColor), stateSetter(NULL) {}
	static void megaIndexEnsure(int count);
	void push(float x1, float y1, float x2, float y2, unsigned int color = WHITE, float *texes = NULL);
	void push(float x1, float y1, float x2, float y2, unsigned int color, texture_slice *s);
	void push(float x1, float y1, float x2, float y2, float r = 1.0, float g = 1.0, float b = 1.0, float a = 1.0, float *texes = NULL);
	void push(float x1, float y1, float x2, float y2, float r, float g, float b, float a, texture_slice *s);
	void push4(cpVect *vertices, unsigned int color = WHITE, float *texes = NULL);
	void push4(cpVect *vertices, unsigned int color, texture_slice *s);
	void draw(); // Implemented in util_display.cpp
};

struct stateoid { virtual void set(const quadpile &) = 0; };

struct linepile : public vector<float> {
	void push(float x1, float y1, float x2, float y2, bool usecolor = false, float r = 1.0, float g = 1.0, float b = 1.0, float a = 1.0);
    void push(cpVect v1, cpVect v2, bool usecolor = false, float r = 1.0, float g = 1.0, float b = 1.0, float a = 1.0);
};

struct pile : public vector<float> {
    void push(cpVect vertex, bool usecolor = false, float r = 1.0, float g = 1.0, float b = 1.0, float a = 1.0, float *texes = NULL);
};

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

static inline cpVect cpvscale(const cpVect v1, const cpVect v2) {
	return cpv(v1.x * v2.x, v1.y * v2.y);
}

static inline cpVect ynorm(const cpVect v1) {
	return cpvmult(v1, 1.0/v1.y);
}

void goOrtho();
void orthoText();

#endif /* _JUMPCORE_DISPLAY */